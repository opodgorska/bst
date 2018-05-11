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

#include <data/datautils.h>
#include <data/processunspent.h>
#include <data/retrievedatatxs.h>
#include <data/storedatatxs.h>

static constexpr size_t maxDataSize=MAX_OP_RETURN_RELAY-6;
static std::string changeAddress("");

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

static std::string getOPreturnData(const std::string& txid)
{
    RetrieveDataTxs retrieveDataTxs(txid);
    return retrieveDataTxs.getTxData();
}

UniValue setOPreturnData(const std::string& hexMsg)
{
    UniValue res(UniValue::VARR);
    
    double fee=computeFee(hexMsg.length());
    
    const CWalletRef pwallet=vpwallets[0];
    std::vector<std::string> addresses;
    ProcessUnspent processUnspent(pwallet, addresses);

    UniValue inputs(UniValue::VARR);
    if(!processUnspent.getUtxForAmount(inputs, fee))
    {
        throw std::runtime_error(std::string("Insufficient funds"));
    }

    if(changeAddress.empty())
    {
        changeAddress=getChangeAddress(pwallet);
    }

    UniValue sendTo(UniValue::VOBJ);                
    sendTo.pushKV(changeAddress, computeChange(inputs, fee));
    sendTo.pushKV("data", hexMsg);

    StoreDataTxs storeDataTxs(pwallet, inputs, sendTo);
    storeDataTxs.signTx();
    std::string txid=storeDataTxs.sendTx().get_str();

    return UniValue(UniValue::VSTR, txid);
}

UniValue retrievedata(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR});
	
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
	throw std::runtime_error(
		"retrievedata \"txid\" \n"
		"\nRetrieves user data from a blockchain.\n"

		"\nArguments:\n"
		"1. \"txid\"                        (string, required) A hex-encoded transaction id string\n"
		"2. \"path to the file\"            (string, optional) A path to the file\n"

		"\nResult:\n"
		"\"string\"                         (string) A retrieved user data string\n"


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
		"1. \"txid\"                        (string, required) A hex-encoded transaction id string\n"

		"\nResult:\n"
		"\"string\"                         (string) A retrieved user data string\n"


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
		"1. \"string\"                      (string, required) A user data string\n"

		"\nResult:\n"
		"\"txid\"                           (string) A hex-encoded transaction id\n"


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
		"1. \"path to the file\"            (string, required) A path to the file\n"

		"\nResult:\n"
		"\"txid\"                           (string) A hex-encoded transaction id\n"


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
		"1. \"path to the file\"            (string, required) A path to the file\n"

		"\nResult:\n"
		"\"txid\"                           (string) A hex-encoded transaction id\n"


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
