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

static void getOpReturnAccReward(double& rewardAcc, const CTransaction& tx, const UniValue& amount)
{
    int reward=0;
    char* rewardPtr=reinterpret_cast<char*>(&reward);
    for(size_t i=2;i<tx.vout[1].scriptPubKey.size();++i)
    {
        *rewardPtr=tx.vout[1].scriptPubKey[i];
        ++rewardPtr;
    }
    rewardAcc+=(reward*amount.get_real());
}

//#define USE_POTENTIAL_REWARD 1
bool checkBetRewardSum(double& rewardAcc, const CTransaction& tx, const Consensus::Params& params, int32_t makeBetIndicator, double accumulatedBetReward)
{
    double blockSubsidy = static_cast<double>(GetBlockSubsidy(chainActive.Height(), params)/COIN);
    int32_t txMakeBetVersion=(tx.nVersion ^ makeBetIndicator);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        UniValue amount(UniValue::VNUM);
        amount=ValueFromAmount(tx.vout[0].nValue);

//to control the potential reward use below commented code
#if defined(USE_POTENTIAL_REWARD)
        getOpReturnAccReward(rewardAcc, tx);
#else
        rewardAcc+=amount.get_real();
#endif
        //LogPrintf("rewardAcc: %f\n", rewardAcc);
        if(rewardAcc>accumulatedBetReward*blockSubsidy)
        {
            LogPrintf("Accumulated reward %f reached the limit\n", rewardAcc);
            return false;
        }
    }
    return true;
}
