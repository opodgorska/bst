// Copyright (c) 2018 Slawek Mozdzonek
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

constexpr size_t maxDataSize=MAX_OP_RETURN_RELAY-6;

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
		for(size_t i=0;i<size;++i)
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

class FileReader
{
public:
	FileReader(const std::string& fileName_) : file(fileName_.c_str(), std::ios::in|std::ios::binary|std::ios::ate)
	{
		if(file.is_open())
		{
			size = file.tellg();
		}		
	}
	
	~FileReader()
	{
		if(file.is_open())
		{
			file.close();
		}
	}
	
	void read(std::vector<char>& binaryData)
	{
		if(file.is_open())
		{
			binaryData.resize(size);
			file.seekg(0, std::ios::beg);
			file.read(binaryData.data(), size);
		}
	}

private:
	std::ifstream file;
	std::streampos size;
};

class FileWriter
{
public:
	FileWriter(const std::string& fileName_) : file(fileName_.c_str(), std::ios::out|std::ios::binary|std::ios::trunc) {}

	~FileWriter()
	{
		if(file.is_open())
		{
			file.close();
		}
	}

	void write(const std::vector<char>& binaryData)
	{
		if(file.is_open())
		{
			file.write(binaryData.data(), binaryData.size());
		}
	}

private:
	std::ofstream file;	
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

static int hexStr2int(const std::string& str)
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

static void reverseEndianess(std::string& str)
{
	std::string tmp=str;
	size_t length=str.length();
	if(length%2)
	{
		std::runtime_error("reverseEndianess input must have even length");
	}
	for(size_t i=0; i<length; i+=2)
	{
		tmp[i]=str[length-i-2];
		tmp[i+1]=str[length-i-1];
	}
	str.swap(tmp);
}

static void hex2bin(std::vector<char>& binaryData, const std::string& hexstr)
{
        char hex_byte[3];
        char *ep;

        hex_byte[2] = '\0';
        int i=0;
        size_t len=hexstr.length();
        for(size_t j=0; j<len; j+=2, i++) 
        {
                if(!hexstr[1]) 
                {
					std::runtime_error("hex2bin str truncated");
                }
                hex_byte[0] = hexstr[j];
                hex_byte[1] = hexstr[j+1];
                binaryData[i] = static_cast<char>(strtol(hex_byte, &ep, 16));
                if(*ep) 
                {
					std::runtime_error("hex2bin failed");
                }
        }
}

static std::string getOPreturnData(const std::string& txid)
{
	UniValue tx(UniValue::VARR);
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
				int order=hexStr2int(hexStr.substr(2,2));
				if(order<=0x4b)
				{
					length=order;
					offset=4;
				}
				else if(order==0x4c)
				{
					length=hexStr2int(hexStr.substr(4,2));
					offset=6;
				}
				else if(order==0x4d)
				{
					std::string strLength=hexStr.substr(4,4);
					reverseEndianess(strLength);
					length=hexStr2int(strLength);
					offset=8;
				}
				else if(order==0x4e)
				{
					std::string strLength=hexStr.substr(4,8);
					reverseEndianess(strLength);
					length=hexStr2int(strLength);
					offset=12;
				}

				length*=2;
				return hexStr.substr(offset, length);
			}
		}
		return std::string("");
	}
	return std::string("");
}

UniValue setOPreturnData(const std::string& hexMsg)
{
    UniValue res(UniValue::VARR);
    UniValue unsignedTx(UniValue::VARR);
    UniValue signedTx(UniValue::VARR);

	size_t dataSize=hexMsg.length()/2;
	constexpr size_t txEmptySize=145;
	constexpr CAmount feeRate=10;
	double fee=static_cast<double>(txEmptySize+(::minRelayTxFee.GetFee(dataSize)*feeRate))/COIN;
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
		throw std::runtime_error("not enough funds or listunspent returned an empty list");
	}

    return res;
}

UniValue retrievedata(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
	throw std::runtime_error(
		"retrievedata \"txid\" \n"
		"\nRetrieves user data from a blockchain.\n"

		"\nArguments:\n"
		"1. \"txid\"						(string, required) A hex-encoded transaction id string\n"
		"2. \"path to the file\"			(string, optional) A path to the file\n"

		"\nResult:\n"
		"\"string\"							(string) A retrieved user data string\n"


		"\nExamples:\n"
		+ HelpExampleCli("retrievedata", "\"txid\"")
		+ HelpExampleRpc("retrievedata", "\"txid\"")
		+ HelpExampleCli("retrievedata", "\"txid\" \"/home/myfile.bin\"")
		+ HelpExampleRpc("retrievedata", "\"txid\" \"/home/myfile.bin\"")
	);

	std::string txid=request.params[0].get_str();
	std::string OPreturnData=getOPreturnData(txid);

	if(!request.params[1].isNull())
	{
		std::string filePath=request.params[1].get_str();
		std::vector<char> OPreturnBinaryData;
		OPreturnBinaryData.resize(OPreturnData.length()/2);
		hex2bin(OPreturnBinaryData, OPreturnData);
		
		FileWriter fileWriter(filePath);
		fileWriter.write(OPreturnBinaryData);

		return UniValue(UniValue::VSTR);
	}
	
	return UniValue(UniValue::VSTR, std::string("\"")+OPreturnData+std::string("\""));
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

	std::string txid=request.params[0].get_str();
	std::string OPreturnData=getOPreturnData(txid);
	if(!OPreturnData.empty())
	{
		std::string asciiStr;
		hex2ascii(OPreturnData, asciiStr);				
		return UniValue(UniValue::VSTR, std::string("\"")+asciiStr+std::string("\""));
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

    std::string msg=request.params[0].get_str();

	if(msg.length()>maxDataSize)
	{
		throw std::runtime_error(strprintf("data size is grater than %d bytes", maxDataSize));
	}

	std::string hexMsg=HexStr(msg.begin(), msg.end());
	
	return setOPreturnData(hexMsg);
}

UniValue storesignature(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 1)
	throw std::runtime_error(
		"storesignature \"string\" \n"
		"\nStores a hash of a user file into a blockchain.\n"
		"A transaction fee is computed as a (hash length)*(fee rate). \n"
		"Before this command walletpassphrase is required. \n"

		"\nArguments:\n"
		"1. \"path to the file\"			(string, required) A path to the file\n"

		"\nResult:\n"
		"\"txid\"							(string) A hex-encoded transaction id\n"


		"\nExamples:\n"
		+ HelpExampleCli("storesignature", "\"/home/myfile.txt\"")
		+ HelpExampleRpc("storesignature", "\"/home/myfile.txt\"")
	);

	UniValue res(UniValue::VARR);

	std::string filePath=request.params[0].get_str();

	std::vector<char> binaryData;
	constexpr size_t hashSize=CSHA256::OUTPUT_SIZE;
	unsigned char fileHash[hashSize];

	FileReader fileReader(filePath);
	fileReader.read(binaryData);

	CHash256 fileHasher;
	
	fileHasher.Write(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size());
	fileHasher.Finalize(fileHash);

	return setOPreturnData(byte2str(fileHash, hashSize));
}

UniValue storedata(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 1)
	throw std::runtime_error(
		"storedata \"string\" \n"
		"\nStores content of a user file into a blockchain.\n"
		"A transaction fee is computed as a (file size)*(fee rate). \n"
		"Before this command walletpassphrase is required. \n"

		"\nArguments:\n"
		"1. \"path to the file\"			(string, required) A path to the file\n"

		"\nResult:\n"
		"\"txid\"							(string) A hex-encoded transaction id\n"


		"\nExamples:\n"
		+ HelpExampleCli("storedata", "\"/home/myfile.txt\"")
		+ HelpExampleRpc("storedata", "\"/home/myfile.txt\"")
	);

	UniValue res(UniValue::VARR);

	std::string filePath=request.params[0].get_str();

	std::vector<char> binaryData;

	FileReader fileReader(filePath);
	fileReader.read(binaryData);
	
	if(binaryData.size()>maxDataSize)
	{
		throw std::runtime_error(strprintf("data size is grater than %d bytes", maxDataSize));
	}

	return setOPreturnData(byte2str(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size()));
}

static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "blockchain",         "storemessage",                	&storemessage,             	{"message"} },
    { "blockchain",         "retrievemessage",             	&retrievemessage,          	{"txid"} },
    { "blockchain",         "retrievedata",             	&retrievedata,          	{"txid"} },
    { "blockchain",         "storesignature",             	&storesignature,          	{"file_path"} },
    { "blockchain",         "storedata",             		&storedata,          		{"file_path"} },
};

void RegisterDataRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
