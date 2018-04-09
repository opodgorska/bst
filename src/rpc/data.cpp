// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/client.h>
#include <validation.h>
#include <policy/policy.h>
#include <utilstrencodings.h>
#include <stdint.h>
#include <amount.h>
#include <hash.h>

#include <univalue.h>
#include <boost/algorithm/string.hpp>

class ProcessListunspent
{
private:
	void parse(const UniValue& arg)
	{
		txid=arg[std::string("txid")].get_str();
		vout=arg[std::string("vout")].get_int();
		amount=arg[std::string("amount")].get_real();
	}
	
	size_t getSize()
	{
		return listunspent.size();
	}

public:
	ProcessListunspent(UniValue listunspent_) : listunspent(listunspent_), fee(0.0) {}
	void setFee(double fee_) {fee=fee_;}
	std::string getTxid() const
	{
		return txid;
	}
	
	int getVout() const
	{
		return vout;
	}
	
	double getAmount() const
	{
		return amount;
	}
	
	bool process()
	{
		size_t size=getSize();
		for(size_t i=0;i<size;++size)
		{
			parse(listunspent[i]);
			if(amount>=fee)
			{
				return true;
			}
		}
		return false;
	}

private:
	UniValue listunspent;
	double fee;
	std::string txid;
	int vout;
	double amount;
};

static UniValue callRPC(std::string args)
{
    std::vector<std::string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    std::string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    JSONRPCRequest request;
    request.strMethod = strMethod;
    request.params = RPCConvertValues(strMethod, vArgs);
    request.fHelp = false;

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        UniValue result = (*method)(request);
        return result;
    }
    catch (const UniValue& objError) {
        throw std::runtime_error(find_value(objError, "message").get_str());
    }
}

static unsigned char hexval(unsigned char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else throw std::runtime_error("hexval decoding failed");
}

static void hex2ascii(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.length() / 2);
    for (std::string::const_iterator p = in.begin(); p != in.end(); p++)
    {
       unsigned char c = hexval(*p);
       p++;
       if (p == in.end()) break;
       c = (c << 4) + hexval(*p);
       out.push_back(c);
    }
}

static int hexStr2Int(const std::string& str)
{
	int ret;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> ret;

	return ret;
}

static std::string byte2str(const unsigned char* binaryData, size_t size)
{
	const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B','C','D','E','F'};
	std::string str;
	for (size_t i = 0; i < size; ++i) 
	{
		const unsigned char ch = binaryData[i];
		str.append(&hex[(ch  & 0xF0) >> 4], 1);
		str.append(&hex[ch & 0xF], 1);
	}
	return str;
}

UniValue retrievemessage(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 1)
	throw std::runtime_error(
		"retrievemessage \"txid\" \n"
		"\nRetrieves user data string from a blockchain.\n"

		"\nArguments:\n"
		"1. \"txid\"						(string, required) A hex-encoded transaction id string\n"

		"\nResult:\n"
		"\"string\"							(string) A retrieved user data string\n"


		"\nExamples:\n"
		+ HelpExampleCli("retrievemessage", "\"txid\"")
		+ HelpExampleRpc("retrievemessage", "\"txid\"")
	);

	UniValue tx(UniValue::VARR);

	std::string txid=request.params[0].get_str();
	tx=callRPC(std::string("getrawtransaction ")+txid+std::string(" 1"));
	
	if(tx.exists(std::string("vout")))
	{		
		UniValue vout=tx[std::string("vout")];

		for(size_t i=0;i<vout.size();++i)
		{
			if(vout[i][std::string("scriptPubKey")][std::string("asm")].get_str().find(std::string("OP_RETURN"))==0)
			{
				int length=0;
				int offset=0;
				std::string hexStr=vout[i][std::string("scriptPubKey")][std::string("hex")].get_str();
				int order=hexStr2Int(hexStr.substr(2,2));
				if(order<=0x4b)
				{
					length=2*order;
					offset=4;
				}
				else if(order==0x4c)
				{
					length=2*hexStr2Int(hexStr.substr(4,2));
					offset=6;
				}
				else if(order==0x4d)
				{
					length=2*hexStr2Int(hexStr.substr(4,4));
					offset=8;
				}

				std::string asciiStr;
				hex2ascii(hexStr.substr(offset, length), asciiStr);				
				return UniValue(UniValue::VSTR, std::string("\"")+asciiStr+std::string("\""));
			}
		}
	}
	
	return UniValue(UniValue::VSTR, std::string("\"\""));
}

UniValue storemessage(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 1)
	throw std::runtime_error(
		"storemessage \"string\" \n"
		"\nStores a user data string in a blockchain.\n"
		"A transaction fee is computed as a (string length)*(fee rate). \n"
		"Before this command walletpassphrase is required. \n"

		"\nArguments:\n"
		"1. \"string\"						(string, required) A user data string\n"

		"\nResult:\n"
		"\"txid\"							(string) A hex-encoded transaction id\n"


		"\nExamples:\n"
		+ HelpExampleCli("storemessage", "\"mystring\"")
		+ HelpExampleRpc("storemessage", "\"mystring\"")
	);

    UniValue res(UniValue::VARR);
    UniValue unsignedTx(UniValue::VARR);
    UniValue signedTx(UniValue::VARR);

    std::string msg=request.params[0].get_str();


	if(msg.size()>80)
	{
		throw std::runtime_error("message to long");
	}

	std::string hexMsg=HexStr(msg.begin(), msg.end());

	double fee=static_cast<double>(::minRelayTxFee.GetFee(msg.size()))/1000;
	res=callRPC(std::string("listunspent"));

	ProcessListunspent processListunspent(res);
	processListunspent.setFee(fee);
	if(processListunspent.process())
	{
		std::string txid=processListunspent.getTxid();
		double change=processListunspent.getAmount()-fee;
		int vout=processListunspent.getVout();
		res=callRPC(std::string("getrawchangeaddress"));

		std::string tx=std::string("[{\"txid\":\"")+txid+std::string("\"")+
					   std::string(",\"vout\":")+std::to_string(vout)+std::string("}]")+
					   std::string(" {\"")+res.get_str()+std::string("\":")+std::to_string(change)+
					   std::string(",\"data\":\"")+hexMsg+std::string("\"}");

		unsignedTx=callRPC(std::string("createrawtransaction ")+tx);

		signedTx=callRPC(std::string("signrawtransactionwithwallet ")+unsignedTx.get_str());

		res=callRPC(std::string("sendrawtransaction ")+signedTx[std::string("hex")].get_str());
	}
	else
	{
		throw std::runtime_error("listunspent returned an empty list");
	}

    return res;
}

class FileReader
{
public:
	FileReader(const std::string& fileName_, std::vector<char>& binaryData) : file(fileName_.c_str(), std::ios::in|std::ios::binary|std::ios::ate)
	{
		if (file.is_open())
		{
			size = file.tellg();
			binaryData.resize(size);
			file.seekg(0, std::ios::beg);
			file.read(binaryData.data(), size);
		}		
	}
	
	~FileReader()
	{
		if (file.is_open())
		{
			file.close();
		}
	}
private:
	std::ifstream file;
	std::streampos size;
};

UniValue storefilehash(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 1)
	throw std::runtime_error(
		"storefilehash \"string\" \n"
		"\nStores a hash of a user file into a blockchain.\n"
		"A transaction fee is computed as a (hash length)*(fee rate). \n"
		"Before this command walletpassphrase is required. \n"

		"\nArguments:\n"
		"1. \"path to the file\"			(string, required) A path to the file\n"

		"\nResult:\n"
		"\"txid\"							(string) A hex-encoded transaction id\n"


		"\nExamples:\n"
		+ HelpExampleCli("storefilehash", "\"/home/myfile.txt\"")
		+ HelpExampleRpc("storefilehash", "\"/home/myfile.txt\"")
	);

	UniValue res(UniValue::VARR);

	std::string filePath=request.params[0].get_str();

	std::vector<char> binaryData;
	constexpr size_t hashSize=CSHA256::OUTPUT_SIZE;
	unsigned char fileHash[hashSize];

	FileReader fileReader(filePath, binaryData);

	CHash256 fileHasher;
	
	fileHasher.Write(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size());
	fileHasher.Finalize(fileHash);

	res=callRPC(std::string("storemessage ")+byte2str(fileHash, hashSize));
	
    return res;
}

static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "blockchain",         "storemessage",                	&storemessage,             	{"message"} },
    { "blockchain",         "retrievemessage",             	&retrievemessage,          	{"txid"} },
    { "blockchain",         "storefilehash",             	&storefilehash,          	{"file_path"} },
};

void RegisterDataRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
