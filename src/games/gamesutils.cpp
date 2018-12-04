// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <logging.h>
#include <rpc/server.h>
#include <uint256.h>
#include <univalue.h>
#include <validation.h>
#include <data/datautils.h>
#include <games/gamesutils.h>

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

ArgumentOperation::ArgumentOperation() : argument(0)
{
};

ArgumentOperation::ArgumentOperation(unsigned int argument_) : argument(argument_)
{
};

void ArgumentOperation::setArgument(unsigned int argument_)
{
    argument=argument_;
};

UniValue findTx(const std::string& txid) {
    return std::get<0>(findTxData(txid));
}

std::tuple<UniValue, CTransactionRef> findTxData(const std::string& txid)
{
    LOCK(cs_main);

    bool in_active_chain = true;
    uint256 hash = ParseHashV(txid, "parameter 1");
    CBlockIndex* blockindex = nullptr;
    CTransactionRef tx;
    uint256 hash_block;
    UniValue result(UniValue::VOBJ);

    int i;
    for(i=chainActive.Height();i>=0;--i)
    {
        blockindex = chainActive[i];

        if (hash == Params().GenesisBlock().hashMerkleRoot)
        {
            // Special exception for the genesis block coinbase transaction
            throw std::runtime_error(std::string("The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved"));
        }

        if (GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex))
        {
            if(blockindex)
            {
                result.pushKV("in_active_chain", in_active_chain);
            }
            TxToJSON(*tx, hash_block, result);
            result.pushKV("blockhash", hash_block.ToString());
            break;
        }
    }

    if(i<0)
    {
        throw std::runtime_error(std::string("Transaction not in blockchain"));
    }

    return std::make_tuple(result, tx);
}

std::tuple<std::string, size_t> getBetData(const UniValue& txPrev) {
    size_t idx;
    std::string betType;

    for (idx=1; idx<txPrev["vout"].size(); ++idx)
    {
        if (txPrev["vout"][idx][std::string("scriptPubKey")][std::string("asm")].get_str().find(std::string("OP_RETURN"))==0)
        {
            int length=0;
            int offset=0;
            std::string hexStr=txPrev["vout"][idx][std::string("scriptPubKey")][std::string("hex")].get_str();
            int order=std::stoi(hexStr.substr(2,2),nullptr,16);
            if (order<=0x4b)
            {
                length=order;
                offset=4;
            }
            else if (order==0x4c)
            {
                length=std::stoi(hexStr.substr(4,2),nullptr,16);
                offset=6;
            }
            else if (order==0x4d)
            {
                std::string strLength=hexStr.substr(4,4);
                reverseEndianess(strLength);
                length=std::stoi(strLength,nullptr,16);
                offset=8;
            }
            else if (order==0x4e)
            {
                std::string strLength=hexStr.substr(4,8);
                reverseEndianess(strLength);
                length=std::stoi(strLength,nullptr,16);
                offset=12;
            }
            else
            {
                LogPrintf("txVerify: betType length is too-large\n");
                throw std::runtime_error(LENGTH_TOO_LARGE);
            }

            length*=2;
            std::string betTypeHex=hexStr.substr(offset, length);
            hex2ascii(betTypeHex, betType);
            break;
        }
    }

    if (idx == txPrev["vout"].size()) {
        throw std::runtime_error(OP_RETURN_NOT_FOUND);
    }

    return std::make_tuple(betType, idx);
}

unsigned int getArgumentFromBetType(std::string& betType)
{
    size_t pos_=betType.find("_");
    if(pos_==std::string::npos)
    {
        LogPrintf("VerifyBlockReward::getArgument() find _ failed");
        throw std::runtime_error(std::string("VerifyBlockReward::getArgument() find _ failed"));
    }
    std::string opReturnArg=betType.substr(0,pos_);
    betType=betType.substr(pos_+1);

    unsigned int opReturnArgNum;
    sscanf(opReturnArg.c_str(), "%x", &opReturnArgNum);

    return opReturnArgNum;
}

unsigned int blockHashStr2Int(const std::string& hashStr)
{
    unsigned int hash;
    std::vector<unsigned char> binaryBlockHash(hashStr.length()/2, 0);
    hex2bin(binaryBlockHash, hashStr);
    std::vector<unsigned char> blockhashVector(binaryBlockHash.end()-4, binaryBlockHash.end());
    array2typeRev(blockhashVector, hash);

    return hash;
}

