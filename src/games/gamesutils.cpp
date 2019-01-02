// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <logging.h>
#include <policy/policy.h>
#include <rpc/server.h>
#include <uint256.h>
#include <univalue.h>
#include <validation.h>

#include <games/gamesutils.h>
#include <games/modulo/modulotxs.h>
#include <games/modulo/moduloverify.h>
#include <data/datautils.h>

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

UniValue findTx(const std::string& txid)
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

    return result;
}

CKeyID getTxKeyID(const CTransaction& tx, int inputIdx)
{
    //TODO what if funding transacton is neither p2pkh nor p2sh-wittness?
    if(!tx.vin[inputIdx].scriptWitness.IsNull())
    {
        return CKeyID(Hash160(tx.vin[inputIdx].scriptWitness.stack[1].begin(), tx.vin[inputIdx].scriptWitness.stack[1].end()));
    }

    return CKeyID(Hash160(tx.vin[inputIdx].scriptSig.end()-33, tx.vin[inputIdx].scriptSig.end()));
}

CScript createScriptPubkey(const CTransaction& prevTx)
{
    const CKeyID keyID = getTxKeyID(prevTx);
    return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
}

static std::string getBetType(const CTransaction& tx)
{
    CScript::const_iterator it_beg=tx.vout[0].scriptPubKey.begin();
    CScript::const_iterator it_end=tx.vout[0].scriptPubKey.end();
    std::string hexStr;
    int order = *(it_beg+1);
    if(*it_beg==OP_RETURN)
    {
        if(order<=0x4b)
        {
            hexStr=std::string(it_beg+2, it_end);
        }
        else if(order==0x4c)
        {
            hexStr=std::string(it_beg+3, it_end);
        }
        else if(order==0x4d)
        {
            hexStr=std::string(it_beg+4, it_end);
        }
        else if(order==0x4e)
        {
            hexStr=std::string(it_beg+6, it_end);
        }
        else
        {
            LogPrintf("getBetType length is too-large\n");
        }
    }
    return hexStr;
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

unsigned int getArgumentFromBetType(std::string& betType, uint max_limit)
{
    size_t pos_=betType.find("_");
    if(pos_==std::string::npos)
    {
        LogPrintf("VerifyBlockReward::getArgumentFromBetType() find _ failed");
        throw std::runtime_error(std::string("VerifyBlockReward::getArgumentFromBetType() find _ failed"));
    }
    std::string opReturnArg=betType.substr(0,pos_);
    betType=betType.substr(pos_+1);

    unsigned int opReturnArgNum;
    sscanf(opReturnArg.c_str(), "%x", &opReturnArgNum);

    if (opReturnArgNum == 0 || opReturnArgNum > max_limit)
    {
        throw std::runtime_error(std::string("VerifyBlockReward::getArgumentFromBetType() incorrect OP_RETURN argument format: " + opReturnArg));
    }

    return opReturnArgNum;
}

MakeBetWinningProcess::MakeBetWinningProcess(const CTransaction& tx, uint256 hash) :
    m_tx(tx),
    m_hash(hash)
{
}

bool MakeBetWinningProcess::isMakeBetWinning()
{
    try{
        //example bet: 00000024_black@200000000+red@100000000
        std::string betType = getBetType(m_tx);
        if (betType.empty()) {
            throw std::runtime_error("Improper bet type");
        }
        LogPrintf("betType = %s\n", betType.c_str());

        const unsigned argument = getArgumentFromBetType(betType);
        LogPrintf("argument = %u\n", argument);

        const std::string blockhashStr = m_hash.ToString();
        LogPrintf("blockhashStr = %s\n", blockhashStr.c_str());

        const unsigned int blockhashTmp = blockHashStr2Int(blockhashStr);
        LogPrintf("blockhashTmp = %u\n", blockhashTmp);

        modulo::ModuloOperation moduloOperation;
        moduloOperation.setArgument(argument);
        const unsigned argumentResult = moduloOperation(blockhashTmp);
        LogPrintf("argumenteResult = %u\n", argumentResult);

        while (true) {
            const size_t typePos = betType.find("@");
            if (typePos == std::string::npos) {
                throw std::runtime_error("Improper bet type");
            }

            const std::string type = betType.substr(0, typePos);
            betType = betType.substr(typePos+1);

            const size_t amountPos = betType.find("+");
            const std::string amountStr = betType.substr(0, amountPos);

            CAmount amount = std::stoll(amountStr);
            if (amount<=0) {
                throw std::runtime_error("Improper bet amount");
            }

            if (modulo::VerifyMakeModuloBetTx().isWinning(type, argument, argumentResult)) {
                const unsigned reward = modulo::GetModuloReward()(type, argument);
                LogPrintf("reward = %u\n", reward);

                const CAmount wonAmount = reward*amount;
                if (wonAmount>MAX_CAMOUNT-m_payoff) {
                    throw std::runtime_error("Improper bet amount");
                }
                m_payoff += wonAmount;
                LogPrintf("%s is winning\n", type.c_str());
            }

            if (amountPos == std::string::npos) {
                break;
            }
            betType = betType.substr(amountPos+1);
        }

        LogPrintf("m_payoff = %d\n", m_payoff);
        if (m_payoff <= 0) {
            return false;
        }

        return true;
    }
    catch(const std::exception& e) {
        LogPrintf("Error while processing bet transaction: %s\n", e.what());
    }
    catch(...) {
        LogPrintf("Unknown error while processing bet transaction\n");
    }
    return false;
}

CAmount MakeBetWinningProcess::getMakeBetPayoff()
{
    return m_payoff;
}

CAmount applyFee(CMutableTransaction& tx, int64_t nTxWeight, int64_t sigOpCost)
{
    if(tx.vout.size()==0)
    {
        return 0;
    }

    //compute fee for this transaction and equally subtract it from each output
    int64_t virtualSize = GetVirtualTransactionSize(nTxWeight, sigOpCost);
    CAmount totalFee = ::minRelayTxFee.GetFee(virtualSize);
    
    CAmount fee = totalFee / tx.vout.size();
    CAmount feeReminder = totalFee % tx.vout.size();
    
    for(auto& txOut : tx.vout)
    {
        if(txOut.nValue>fee)
        {
            txOut.nValue-=fee;
        }
        else
        {
            totalFee-=fee;
            txOut.nValue=0;
        }
    }

    auto& txOut = tx.vout.back();
    if(txOut.nValue>feeReminder)
    {
        txOut.nValue-=feeReminder;
    }
    else
    {
        totalFee-=feeReminder;
        txOut.nValue=0;
    }
    
    return totalFee;
}
