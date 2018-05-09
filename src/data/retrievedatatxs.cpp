// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>

#include <key_io.h>
#include <net.h>

#include <rpc/rawtransaction.h>
#include <rpc/safemode.h>
#include <rpc/server.h>
#include <validation.h>
#include "retrievedatatxs.h"

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

RetrieveDataTxs::RetrieveDataTxs(const std::string& txid, const std::string& blockHash) : transaction(UniValue::VOBJ)
{
    LOCK(cs_main);

    bool in_active_chain = true;
    uint256 hash = uint256S(txid);
    CBlockIndex* blockindex = nullptr;

    if (hash == Params().GenesisBlock().hashMerkleRoot) 
    {
        // Special exception for the genesis block coinbase transaction
        throw std::runtime_error(std::string("The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved"));
    }

    if (!blockHash.empty()) 
    {
        uint256 blockhash = uint256S(blockHash);
        BlockMap::iterator it = mapBlockIndex.find(blockhash);
        if (it == mapBlockIndex.end()) 
        {
            throw std::runtime_error(std::string("Block hash not found"));
        }
        blockindex = it->second;
        in_active_chain = chainActive.Contains(blockindex);
    }

    CTransactionRef tx;
    uint256 hash_block;
    if (!GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex)) 
    {
        std::string errmsg;
        if (blockindex) 
        {
            if (!(blockindex->nStatus & BLOCK_HAVE_DATA)) 
            {
                throw std::runtime_error(std::string("Block not available"));
            }
            errmsg = "No such transaction found in the provided block";
        } 
        else 
        {
            errmsg = fTxIndex
              ? "No such mempool or blockchain transaction"
              : "No such mempool transaction. Use -txindex to enable blockchain transaction queries";
        }
        throw std::runtime_error(errmsg + std::string(". Use gettransaction for wallet transactions."));
    }

    if (blockindex) 
    {
        transaction.pushKV("in_active_chain", in_active_chain);
    }
    TxToJSON(*tx, hash_block, transaction);
}

RetrieveDataTxs::~RetrieveDataTxs() {}

std::string RetrieveDataTxs::getTxData()
{
    if(transaction.exists(std::string("vout")))
	{		
		UniValue vout=transaction[std::string("vout")];

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

int RetrieveDataTxs::hexStr2int(const std::string& str)
{
	int ret;
	std::stringstream ss;
	ss << std::hex << str;
	ss >> ret;

	return ret;
}

void RetrieveDataTxs::reverseEndianess(std::string& str)
{
	std::string tmp=str;
	size_t length=str.length();
	if(length%2)
	{
            throw std::runtime_error("reverseEndianess input must have even length");
	}
	for(size_t i=0; i<length; i+=2)
	{
		tmp[i]=str[length-i-2];
		tmp[i+1]=str[length-i-1];
	}
	str.swap(tmp);
}
