// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <games/gamesutils.h>
#include <games/gamesverify.h>

VerifyBlockReward::VerifyBlockReward(const Consensus::Params& params, const CBlock& block_, ArgumentOperation* argumentOperation_, GetReward* getReward_, VerifyMakeBetTx* verifyMakeBetTx_, int32_t makeBetIndicator_, CAmount maxPayoff_) : 
                                     block(block_), argumentOperation(argumentOperation_), getReward(getReward_), verifyMakeBetTx(verifyMakeBetTx_), makeBetIndicator(makeBetIndicator_), maxPayoff(maxPayoff_)
{
    blockSubsidy=GetBlockSubsidy(chainActive.Height(), params);
    uint256 hash=block.GetHash();
    blockHash=blockHashStr2Int(hash.ToString());
}

unsigned int VerifyBlockReward::getArgument(std::string& betType)
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

bool VerifyBlockReward::isBetPayoffExceeded()
{
    //bioinfo hardfork due to roulette bets definition change
    if(chainActive.Height() < ROULETTE_NEW_DEFS)
    {
        return false;
    }

    CAmount inAcc=0;
    CAmount payoffAcc=0;
    for (const auto& tx : block.vtx)
    {
        try
        {
            if(isBetTx(*tx, makeBetIndicator))
            {
                size_t idx;
                std::string betType=getBetType(*tx, idx);
                if(betType.empty())
                {
                    LogPrintf("isBetPayoffExceeded: empty betType");
                    continue;
                }

                unsigned int argument=getArgument(betType);
                argumentOperation->setArgument(argument);
                argumentResult=(*argumentOperation)(blockHash);
                for(size_t i=0;true;++i)//all tx.otputs
                {
                    size_t pos=betType.find("+");
                    if(verifyMakeBetTx->isWinning(betType.substr(0,pos), argument, argumentResult))
                    {
                        int reward=(*getReward)(betType.substr(0,pos), argument);
                        CAmount payoff=tx->vout[i].nValue * reward;
                        payoffAcc+=payoff;
                    }
                    inAcc+=tx->vout[i].nValue;

                    if(pos==std::string::npos)
                    {
                        break;
                    }
                    betType=betType.substr(pos+1);
                }
            }
        }
        catch(...)
        {
            LogPrintf("isBetPayoffExceeded: argumentOperation failed");
            continue;
        }
    }

    if(inAcc >= ((9*blockSubsidy)/10))
    {
        if(payoffAcc>inAcc+blockSubsidy)
        {
            LogPrintf("payoffAcc: %d, inAcc: %d, blockSubsidy: %d\n", payoffAcc, inAcc, blockSubsidy);
            return true;
        }
    }

    if(payoffAcc>maxPayoff)
    {
        LogPrintf("payoffAcc: %d, maxPayoff: %d\n", payoffAcc, maxPayoff);
        return true;
    }

    return false;
}

bool isBetTx(const CTransaction& tx, int32_t makeBetIndicator)
{
    int32_t txMakeBetVersion=(tx.nVersion ^ makeBetIndicator);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        return true;
    }
    return false;
}
