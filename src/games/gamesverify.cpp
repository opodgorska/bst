// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <games/gamesutils.h>
#include <games/gamestxs.h>
#include <games/gamesverify.h>

static unsigned int getMakeTxBlockHash(const std::string& makeTxBlockHash, unsigned int argument, ArgumentOperation* operation)
{
    unsigned int blockhashTmp=blockHashStr2Int(makeTxBlockHash);
    operation->setArgument(argument);
    return (*operation)(blockhashTmp);
}

bool txVerify(int nSpendHeight, const CTransaction& tx, CAmount in, CAmount out, CAmount& fee, ArgumentOperation* operation, GetReward* getReward, CompareBet2Vector* compareBet2Vector, int32_t indicator, CAmount maxPayoff, int32_t maxReward)
{
    fee=0;
    CAmount totalReward = 0;
    CAmount inputSum = 0;

    for (unsigned int idx=0; idx<tx.vin.size(); ++idx) {
        UniValue txPrev(UniValue::VOBJ);
        CTransactionRef txPrevRef;
        try
        {
            std::tie(txPrev, txPrevRef)=findTxData(tx.vin[idx].prevout.hash.GetHex());
        }
        catch(...)
        {
            LogPrintf("txVerify findTxData() failed\n");
            return false;
        }
        int32_t txVersion=txPrev["version"].get_int();
        int32_t makeBetIndicator = txVersion ^ indicator;
        if(makeBetIndicator > CTransaction::MAX_STANDARD_VERSION || makeBetIndicator < 1)
        {
            return false;
        }
        std::string blockhashStr=txPrev["blockhash"].get_str();

        CScript redeemScript(tx.vin[idx].scriptSig.begin(), tx.vin[idx].scriptSig.end());
        unsigned int argument = getArgument(redeemScript);
        unsigned int blockhash=getMakeTxBlockHash(blockhashStr, argument, operation);

        CScript::const_iterator it_beg=tx.vin[idx].scriptSig.begin()+1;
        std::vector<unsigned char> blockhashFromScript_(it_beg, it_beg+4);

        unsigned int blockhashFromScript;
        array2type(blockhashFromScript_, blockhashFromScript);

        if(blockhash!=blockhashFromScript)
        {
            LogPrintf("txVerify: blockhash-mismatch\n");
            return false;
        }

        std::string betType;
        try
        {
            bool isOpReturnFlag=false;
            for(size_t i=1; i<txPrev["vout"].size();++i)
            {
                if(txPrev["vout"][i][std::string("scriptPubKey")][std::string("asm")].get_str().find(std::string("OP_RETURN"))==0)
                {
                    int length=0;
                    int offset=0;
                    std::string hexStr=txPrev["vout"][i][std::string("scriptPubKey")][std::string("hex")].get_str();
                    int order=std::stoi(hexStr.substr(2,2),nullptr,16);
                    if(order<=0x4b)
                    {
                        length=order;
                        offset=4;
                    }
                    else if(order==0x4c)
                    {
                        length=std::stoi(hexStr.substr(4,2),nullptr,16);
                        offset=6;
                    }
                    else if(order==0x4d)
                    {
                        std::string strLength=hexStr.substr(4,4);
                        reverseEndianess(strLength);
                        length=std::stoi(strLength,nullptr,16);
                        offset=8;
                    }
                    else if(order==0x4e)
                    {
                        std::string strLength=hexStr.substr(4,8);
                        reverseEndianess(strLength);
                        length=std::stoi(strLength,nullptr,16);
                        offset=12;
                    }
                    else
                    {
                        LogPrintf("txVerify: betType length is too-large\n");
                        return false;
                    }

                    length*=2;
                    std::string betTypeHex=hexStr.substr(offset, length);
                    hex2ascii(betTypeHex, betType);
                    isOpReturnFlag=true;
                    break;
                }
            }
            if(!isOpReturnFlag)
            {
                LogPrintf("txVerify: betType length is empty\n");
                return false;
            }
            else
            {
                size_t pos_=betType.find("_");
                std::string opReturnArg=betType.substr(0,pos_);
                betType=betType.substr(pos_+1);

                unsigned int opReturnArgNum;
                sscanf(opReturnArg.c_str(), "%x", &opReturnArgNum);
                if(opReturnArgNum != argument)
                {
                    LogPrintf("txVerify: opReturnArgNum: %x != argument: %x \n", opReturnArgNum, argument);
                    return false;
                }

                uint32_t nPrevOut=tx.vin[idx].prevout.n;
                for(uint32_t n=0;n<nPrevOut;++n)
                {
                    size_t pos=betType.find("+");
                    betType=betType.substr(pos+1);
                }
                size_t pos=betType.find("+");
                betType=betType.substr(0,pos);
            }
        }
        catch(const std::out_of_range& oor)
        {
            LogPrintf("txVerify: betType is out-of-range\n");
            return false;
        }
        if(betType.empty())
        {
            LogPrintf("txVerify: betType is empty\n");
            return false;
        }

        int opReturnReward=(*getReward)(betType, argument);

        int numOfBetsNumbers=0;
        CScript::const_iterator it_end=tx.vin[idx].scriptSig.end()-1;

        if(*it_end!=OP_DROP)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        it_end-=6;
        if(*it_end!=OP_ENDIF)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        if(*it_end!=OP_FALSE)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        if(*it_end!=OP_DROP)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        if(*it_end!=OP_ELSE)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        for(CScript::const_iterator it=it_end;it>it_end-18;--it)
        {
            if(OP_TRUE==*it)
            {
                numOfBetsNumbers=it_end-it+1;
                it_end=it;
                break;
            }
            else if(OP_ENDIF!=*it)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
        }
        --it_end;
        if(*it_end!=OP_EQUALVERIFY)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }

        const int scriptReward=argument/numOfBetsNumbers;
        if(opReturnReward != scriptReward)
        {
            LogPrintf("txVerify: opReturnReward != scriptReward\n");
            return false;
        }

        std::vector<unsigned char> betNumber_(it_end-4, it_end);
        int betNumber=0;
        array2type(betNumber_, betNumber);
        std::vector<int> betNumbers(1, betNumber);
        it_end-=6;

        for(int i=0;i<numOfBetsNumbers-1;++i)
        {
            //OP_DUP << betNumberArray << OP_EQUAL << OP_IF << OP_DROP << OP_TRUE << OP_ELSE
            if(*it_end!=OP_ELSE)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;
            if(*it_end!=OP_TRUE)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;
            if(*it_end!=OP_DROP)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;
            if(*it_end!=OP_IF)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;
            if(*it_end!=OP_EQUAL)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;
            betNumber_[3]=*it_end--;
            betNumber_[2]=*it_end--;
            betNumber_[1]=*it_end--;
            betNumber_[0]=*it_end--;
            --it_end;
            if(*it_end!=OP_DUP)
            {
                LogPrintf("txVerify: transaction format check failed\n");
                return false;
            }
            --it_end;

           array2type(betNumber_, betNumber);
           betNumbers.push_back(betNumber);
        }

        if(!(*compareBet2Vector)(nSpendHeight, betType, betNumbers))
        {
            LogPrintf("txVerify: compareBet2Vector check failed\n");
            return false;
        }

        if(*it_end!=OP_IF)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        if(*it_end!=OP_CHECKSIG)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        --it_end;
        if(*it_end!=OP_EQUALVERIFY)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }

        if(*(it_end-22)!=OP_HASH160)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }
        if(*(it_end-23)!=OP_DUP)
        {
            LogPrintf("txVerify: transaction format check failed\n");
            return false;
        }

        CScript::const_iterator it_begin=tx.vin[idx].scriptSig.begin();
        it_begin+=(*it_begin)+1;
        it_begin+=(*it_begin)+1;
        it_begin+=(*it_begin)+1;
        if(*it_begin==0x4c)
        {
            it_begin++;
        }
        it_begin++;

        if((it_end-23)!=it_begin)
        {
            LogPrintf("txVerify: script length check failed\n");
            return false;
        }

        const uint32_t outIdx = tx.vin[idx].prevout.n;
        const CAmount amount = txPrevRef->vout[outIdx].nValue;
        totalReward += (scriptReward*amount);
        inputSum += amount;

        if(opReturnReward>maxReward || scriptReward>maxReward)
        {
            LogPrintf("txVerify: maxReward exceeded\n");
            return false;
        }
    }

    if (in != inputSum) {
        LogPrintf("txVerify: in != inputSum\n");
        return false;
    }

    if(out>maxPayoff)
    {
        LogPrintf("txVerify: out > maxPayoff\n");
        return false;
    }

    if(out > totalReward)
    {
        LogPrintf("txVerify: out > totalReward\n");
        return false;
    }

    fee = totalReward-out;
    return true;
}

VerifyBlockReward::VerifyBlockReward(const Consensus::Params& params, const CBlock& block_, ArgumentOperation* argumentOperation_, GetReward* getReward_, VerifyMakeBetTx* verifyMakeBetTx_, int32_t makeBetIndicator_, CAmount maxPayoff_) :
                                     block(block_), argumentOperation(argumentOperation_), getReward(getReward_), verifyMakeBetTx(verifyMakeBetTx_), makeBetIndicator(makeBetIndicator_), maxPayoff(maxPayoff_)
{
    blockSubsidy=GetBlockSubsidy(chainActive.Height(), params);
    uint256 hash=block.GetHash();
    blockHash=blockHashStr2Int(hash.ToString());
}

static std::string getBetType_(const CTransaction& tx, size_t& idx)
{
    idx=0;
    for(size_t i=1;i<tx.vout.size();++i)
    {
        CScript::const_iterator it_beg=tx.vout[i].scriptPubKey.begin();
        CScript::const_iterator it_end=tx.vout[i].scriptPubKey.end();
        std::string hexStr;
        std::string lengthStr;
        int order = *(it_beg+1);
        unsigned int length = 0;
        if(*it_beg==OP_RETURN)
        {
            lengthStr = std::string(it_beg, it_beg + 4);
            if(order<=0x4b)
            {
                hexStr=std::string(it_beg+2, it_end);
                memcpy((char*)&length, lengthStr.substr(1).c_str(), 1);
            }
            else if(order==0x4c)
            {
                hexStr=std::string(it_beg+3, it_end);
                memcpy((char*)&length, lengthStr.substr(2).c_str(), 1);
            }
            else if(order==0x4d)
            {
                hexStr=std::string(it_beg+4, it_end);
                memcpy((char*)&length, lengthStr.substr(2).c_str(), 2);
            }
            else if(order==0x4e)
            {
                hexStr=std::string(it_beg+6, it_end);
                memcpy((char*)&length, lengthStr.substr(2).c_str(), 4);
            }
            else
            {
                LogPrintf("getBetType length is too-large\n");
                return std::string("");
            }
            //LogPrintf("getBetType: %s\n", hexStr);
            idx=i;
            if (hexStr.length() != length)
            {
                LogPrintf("%s ERROR: length difference %u, script: %s\n", length, hexStr.c_str());
                return std::string("");
            }
            return hexStr;
        }
    }
    LogPrintf("getBetType no op-return\n");
    return std::string("");
}

std::string getBetType(const CTransaction& tx)
{
    size_t idx;
    return getBetType_(tx, idx);
}

bool VerifyBlockReward::isMakeBetTx(const CTransaction& tx)
{
    int32_t txMakeBetVersion=(tx.nVersion ^ makeBetIndicator);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        return true;
    }
    return false;
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
    bool bidExceededSubsidy = false;
    for (const auto& tx : block.vtx)
    {
        try
        {
            if(isMakeBetTx(*tx))
            {
                std::string betType=getBetType(*tx);
                if(betType.empty())
                {
                    LogPrintf("isBetPayoffExceeded: empty betType");
                    continue;
                }

                unsigned int argument=getArgumentFromBetType(betType);
                argumentOperation->setArgument(argument);
                argumentResult=(*argumentOperation)(blockHash);
                for(size_t i=0;true;++i)//all tx.otputs
                {
                    size_t pos=betType.find("+");
                    int reward=(*getReward)(betType.substr(0,pos), argument);
                    if(verifyMakeBetTx->isWinning(betType.substr(0,pos), argument, argumentResult))
                    {
                        CAmount payoff=tx->vout[i].nValue * reward;
                        payoffAcc+=payoff;
                    }
                    inAcc+=tx->vout[i].nValue;
                    if(reward >= blockSubsidy/2) bidExceededSubsidy = true;

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

        if (bidExceededSubsidy)
        {
            LogPrintf("Potential reward of one bid higher than half subsidy value, blockSubsidy: %d\n", blockSubsidy);
            return true;
        }

        if(payoffAcc>inAcc+blockSubsidy)
        {
            LogPrintf("payoffAcc: %d, inAcc: %d, blockSubsidy: %d\n", payoffAcc, inAcc, blockSubsidy);
            return true;
        }
    }

    return false;
}

bool VerifyBlockReward::checkPotentialRewardLimit(CAmount &rewardSum, const CTransaction &txn, bool ignoreHardfork)
{
    if (!ignoreHardfork && chainActive.Height() < MAKEBET_REWARD_LIMIT)
    {
        return true;
    }

    if (isMakeBetTx(txn))
    {
        std::string betType = getBetType(txn);
        if (betType.empty())
        {
            LogPrintf("%s: Bet type empty\n", __func__);
            return false;
        }
        unsigned int argument = getArgumentFromBetType(betType);

        for (uint i=0; true; ++i)
        {
            size_t pos =  betType.find("+");
            int reward = (*getReward)(betType.substr(0, pos), argument);
            rewardSum += (reward * txn.vout[i].nValue);

            if (pos == std::string::npos)
            {
                break;
            }
            betType = betType.substr(pos+1);
        }

        bool rv = (rewardSum <= maxPayoff);
        if (!rv)
        {
            LogPrintf("%s: ERROR potential:%ld max:%ld\n", __func__, rewardSum, maxPayoff);
        }
        return rv;
    }
    return true;
}

bool isMakeBetTx(const CTransaction& tx, int32_t makeBetIndicator)
{
    int32_t txMakeBetVersion=(tx.nVersion ^ makeBetIndicator);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        return true;
    }
    return false;
}

VerifyMakeBetFormat::VerifyMakeBetFormat(GetReward *getReward, int32_t makeBetIndicator, CAmount maxReward, CAmount maxPayoff)
    : m_getReward(getReward), m_indicator(makeBetIndicator), m_maxReward(maxReward), m_maxPayoff(maxPayoff)
{
}

bool is_lottery(const std::string& betStr)
{
    return betStr.find_first_not_of("0123456789") == std::string::npos;
}

bool VerifyMakeBetFormat::checkBetAmountLimit(int mod_argument, const std::string& bet_type)
{
    if (is_lottery(bet_type))
    {
        unsigned int betAmount = 0;
        try {
            betAmount = std::stoi(bet_type);
        } catch (...) {}
        if (betAmount == 0)
        {
            LogPrintf("%s:ERROR bet amount below limit %u\n", __func__, betAmount);
            return false;
        }
        if (betAmount > mod_argument)
        {
            LogPrintf("%s:ERROR bet amount: %u above game limit %u\n", __func__, betAmount, mod_argument);
            return false;
        }
    } else
    {
        size_t pos_ = bet_type.find_last_of("_");
        if (pos_ != std::string::npos && pos_ < bet_type.length()-1)
        {
            unsigned int betAmount = std::stoi(bet_type.substr(pos_+1, std::string::npos));
            if (betAmount == 0)
            {
                LogPrintf("%s:ERROR bet amount below limit %u\n", __func__, betAmount);
                return false;
            }
        }
    }
    return true;
}

bool VerifyMakeBetFormat::txMakeBetVerify(const CTransaction& tx, bool ignoreHardfork)
{
    //bioinfo hardfork due to incorrect format of makebet transactions
    if(!ignoreHardfork && chainActive.Height() < MAKEBET_FORMAT_VERIFY)
    {
        return true;
    }

    if(tx.vout.size()<2)
    {
        LogPrintf("txMakeBetVerify: tx.size too small: %d\n", tx.vout.size());
        return false;
    }

    if (!isMakeBetTx(tx, m_indicator))
    {
        return true;
    }

    size_t opReturnIdx;
    std::string betType = getBetType_(tx, opReturnIdx);
    if(betType.empty())
    {
        LogPrintf("%s:ERROR betType is empty\n", __func__);
        return false;
    }

    unsigned int argument = getArgumentFromBetType(betType, m_maxReward);
    if (argument > m_maxReward)
    {
        LogPrintf("%s:ERROR bad argument: %ld\n", __func__, argument);
        return false;
    }

    for (uint i=0; true; ++i)
    {
        size_t pos =  betType.find("+");
        int reward = (*m_getReward)(betType.substr(0, pos), argument);
        if (reward == 0)
        {
            LogPrintf("%s:ERROR unknown bet type %s\n", __func__, betType.c_str());
            return false;
        }

        if (!checkBetAmountLimit(argument, betType.substr(0, pos)))
        {
            return false;
        }

        if (tx.vout[i].nValue == 0)
        {
            LogPrintf("%s:ERROR amount below limit %u\n", __func__, tx.vout[i].nValue);
            return false;
        }

        if (pos == std::string::npos)
        {
            break;
        }
        betType = betType.substr(pos+1);
    }

   for(size_t i=0;i<opReturnIdx;++i)
   {
       if(!tx.vout[i].scriptPubKey.IsPayToScriptHash(false))
       {
           LogPrintf("txMakeBetVerify: not P2SH before opReturn\n");
           return false;
       }
   }

    return true;
}
