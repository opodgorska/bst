// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <games/gamesutils.h>
#include <games/gamestxs.h>
#include <games/gamesverify.h>

static unsigned int getMakeTxBlockHash(const std::string& makeTxBlockHash, unsigned int argument, ArgumentOperation* operation)
{
    std::vector<unsigned char> binaryBlockHash(makeTxBlockHash.length()/2, 0);
    hex2bin(binaryBlockHash, makeTxBlockHash);
    std::vector<unsigned char> blockhash(binaryBlockHash.end()-4, binaryBlockHash.end());

    unsigned int blockhashTmp;
    array2typeRev(blockhash, blockhashTmp);
    
    operation->setArgument(argument);
    return (*operation)(blockhashTmp);
}

bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee, ArgumentOperation* operation, GetReward* getReward, int32_t indicator, CAmount maxPayoff, int32_t maxReward)
{
    fee=0;
    UniValue txPrev(UniValue::VOBJ);
    try
    {
        txPrev=findTx(tx.vin[0].prevout.hash.GetHex());
    }
    catch(...)
    {
        LogPrintf("txVerify findTx() failed\n");
        return false;
    }
    int32_t txVersion=txPrev["version"].get_int();
    int32_t makeBetIndicator = txVersion ^ indicator;
    if(makeBetIndicator > CTransaction::MAX_STANDARD_VERSION || makeBetIndicator < 1)
    {
        return false;
    }

    std::string blockhashStr=txPrev["blockhash"].get_str();
    CScript redeemScript(tx.vin[0].scriptSig.begin(), tx.vin[0].scriptSig.end());
    unsigned int argument = getArgument(redeemScript);
    unsigned int blockhash=getMakeTxBlockHash(blockhashStr, argument, operation);

    CScript::const_iterator it_beg=tx.vin[0].scriptSig.begin()+1;
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
                //LogPrintf("txVerify: betType: %s\n", betType);
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
            
            uint32_t nPrevOut=tx.vin[0].prevout.n;
            for(uint32_t n=0;n<nPrevOut;++n)
            {
                size_t pos=betType.find("+");
                betType=betType.substr(pos+1);
            }
            size_t pos=betType.find("+");
            betType=betType.substr(0,pos);
            //LogPrintf("txVerify: betType: %s\n", betType);
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

    //LogPrintf("txVerify opReturnReward: %d\n", opReturnReward);

    int numOfBetsNumbers=0;
    CScript::const_iterator it_end=tx.vin[0].scriptSig.end()-1;

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
    //LogPrintf("txVerify scriptReward: %d\n", scriptReward);
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

    //OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG << OP_IF;
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

    CScript::const_iterator it_begin=tx.vin[0].scriptSig.begin();
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
    
    fee=(scriptReward*in)-out;
    if(out > scriptReward*in)
    {
        LogPrintf("txVerify: out > scriptReward*in\n");
        return false;
    }
    
    if(out>maxPayoff)
    {
        LogPrintf("txVerify: out > maxPayoff\n");
        return false;        
    }
    
    if(opReturnReward>maxReward || scriptReward>maxReward)
    {
        LogPrintf("txVerify: maxReward exceeded\n");
        return false;        
    }

    return true;
}
