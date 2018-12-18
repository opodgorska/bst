// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <games/gamesverify.h>
#include <games/modulo/moduloverify.h>
#include <games/modulo/modulotxs.h>
#include <games/modulo/moduloutils.h>

namespace modulo
{

    int GetModuloReward::operator()(const std::string& betType, unsigned int modulo)
    {
        if(betType.find("straight_")==0 || betType.find("STRAIGHT_")==0 || (betType.find_first_not_of( "0123456789" ) == std::string::npos))
        {
            return modulo;
        }
        else if((betType.find("split_")==0 || betType.find("SPLIT_")==0) && modulo==36)
        {
            return modulo/2;
        }
        else if((betType.find("street_")==0 || betType.find("STREET_")==0) && modulo==36)
        {
            return modulo/3;
        }
        else if((betType.find("corner_")==0 || betType.find("CORNER_")==0) && modulo==36)
        {
            return modulo/4;
        }
        else if((betType.find("line_")==0 || betType.find("LINE_")==0) && modulo==36)
        {
            return modulo/6;
        }
        else if((betType.find("column_")==0 || betType.find("COLUMN_")==0) && modulo==36)
        {
            return modulo/12;
        }
        else if((betType.find("dozen_")==0 || betType.find("DOZEN_")==0) && modulo==36)
        {
            return modulo/12;
        }
        else if((betType.find("low")==0 || betType.find("LOW")==0) && modulo==36)
        {
            return modulo/18;
        }
        else if((betType.find("high")==0 || betType.find("HIGH")==0) && modulo==36)
        {
            return modulo/18;
        }
        else if((betType.find("even")==0 || betType.find("EVEN")==0) && modulo==36)
        {
            return modulo/18;
        }
        else if((betType.find("odd")==0 || betType.find("ODD")==0) && modulo==36)
        {
            return modulo/18;
        }
        else if((betType.find("red")==0 || betType.find("RED")==0) && modulo==36)
        {
            return modulo/18;
        }
        else if((betType.find("black")==0 || betType.find("BLACK")==0) && modulo==36)
        {
            return modulo/18;
        }
        return 0;
    }

    bool VerifyMakeModuloBetTx::isWinning(const std::string& betType, unsigned int maxArgument, unsigned int argument)
    {
        int* bet;
        int len=1;
        if(betType.find_first_not_of( "0123456789" ) == std::string::npos)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType);
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>maxArgument)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: ")+std::to_string(maxArgument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            straight = betNum;
            bet = const_cast<int* >(&straight);
            len = 1;
        }
        else if((betType.find("straight_")==0 || betType.find("STRAIGHT_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("straight_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>36)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            straight = betNum;
            bet = const_cast<int* >(&straight);
            len = 1;
        }
        else if((betType.find("split_")==0 || betType.find("SPLIT_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("split_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>57)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(split[betNum-1]);
            len = 2;
        }
        else if((betType.find("street_")==0 || betType.find("STREET_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("street_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }        
            if(betNum<1 || betNum>12)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(street[betNum-1]);
            len = 3;
        }
        else if((betType.find("corner_")==0 || betType.find("CORNER_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("corner_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>22)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(corner[betNum-1]);
            len = 4;
        }
        else if((betType.find("line_")==0 || betType.find("LINE_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("line_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>11)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(line[betNum-1]);
            len = 6;
        }
        else if((betType.find("column_")==0 || betType.find("COLUMN_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("column_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>3)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(column[betNum-1]);
            len = 12;
        }
        else if((betType.find("dozen_")==0 || betType.find("DOZEN_")==0) && maxArgument==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType.substr(std::string("dozen_").length()));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" betNum: ")+std::to_string(betNum));
            }
            if(betNum<1 || betNum>3)
            {
                throw std::runtime_error(std::string("VerifyMakeModuloBetTx::isWinning argument failed: ")+std::to_string(argument)+std::string(" maxArgument: 36")+std::string(" betNum: ")+std::to_string(betNum));
            }
            bet = const_cast<int* >(dozen[betNum-1]);
            len = 12;
        }
        else if((betType.find("low")==0 || betType.find("LOW")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(low);
            len = 18;
        }
        else if((betType.find("high")==0 || betType.find("HIGH")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(high);
            len = 18;
        }
        else if((betType.find("even")==0 || betType.find("EVEN")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(even);
            len = 18;
        }
        else if((betType.find("odd")==0 || betType.find("ODD")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(odd);
            len = 18;
        }
        else if((betType.find("red")==0 || betType.find("RED")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(red);
            len = 18;
        }
        else if((betType.find("black")==0 || betType.find("BLACK")==0) && maxArgument==36)
        {
            bet = const_cast<int* >(black);
            len = 18;
        }
        else
        {
            return false;
        }

        int *item = std::find(bet, bet+len, argument);
        if (item != bet+len) 
        {
            return true;
        }
        return false;
    }

    bool CompareModuloBet2Vector::operator()(int nSpendHeight, const std::string& betTypePattern, const std::vector<int>& betNumbers)
    {
        //bioinfo hardfork due to roulette bets definition change
        if(nSpendHeight < ROULETTE_NEW_DEFS)
        {
            return true;
        }

        std::vector<int> opReturnBet;
        if(!bet2Vector(betTypePattern, opReturnBet))
        {
            LogPrintf("CompareModuloBet2Vector::operator(): bet2Vector() failed");
            return false;
        }

        try
        {
            std::reverse(std::begin(opReturnBet), std::end(opReturnBet));
        }
        catch(...)
        {
            LogPrintf("CompareModuloBet2Vector::operator(): std::reverse() failed");
            return false;
        }

        if(betNumbers.size()!=opReturnBet.size())
        {
            LogPrintf("CompareModuloBet2Vector: vectors size mismatch\n");
            return false;
        }

        if(!std::equal( betNumbers.begin(), betNumbers.end(), opReturnBet.begin() ))
        {
            LogPrintf("CompareModuloBet2Vector: vectors are different\n");
            LogPrintf("betTypePattern: %s\n", betTypePattern);

            for(size_t i=0;i<betNumbers.size();++i)
            {
                LogPrintf("betNumbers: %d, %d\n", betNumbers[i], opReturnBet[i]);
            }
            LogPrintf("\n");

            return false;
        }

        return true;
    }

    bool isMakeBetTx(const CTransaction& tx)
    {
        return isMakeBetTx(tx, MAKE_MODULO_GAME_INDICATOR);
    }

    bool isNewMakeBetTx(const CTransaction& tx)
    {
        return isMakeBetTx(tx, MAKE_MODULO_NEW_GAME_INDICATOR);
    }

    bool txVerify(int nSpendHeight, const CTransaction& tx, CAmount in, CAmount out, CAmount& fee)
    {
        try
        {
            ModuloOperation moduloOperation;
            GetModuloReward getModuloReward;
            CompareModuloBet2Vector compareModulobet2Vector;
            return txVerify(nSpendHeight, tx, in, out, fee, &moduloOperation, &getModuloReward, &compareModulobet2Vector, MAKE_MODULO_GAME_INDICATOR, MAX_PAYOFF, MAX_REWARD);
        }
        catch(...)
        {
            LogPrintf("modulo::txVerify exception occured");
            return false;
        }
    };

    bool isBetPayoffExceeded(const Consensus::Params& params, const CBlock& block)
    {
        try
        {
            VerifyMakeModuloBetTx verifyMakeModuloBetTx;
            ModuloOperation moduloOperation;
            GetModuloReward getModuloReward;
            VerifyBlockReward verifyBlockReward(params, block, &moduloOperation, &getModuloReward, &verifyMakeModuloBetTx, MAKE_MODULO_GAME_INDICATOR, MAX_PAYOFF);
            return verifyBlockReward.isBetPayoffExceeded();
        }
        catch(...)
        {
            LogPrintf("modulo::isBetPayoffExceeded exception occured");
            return false;
        }
    };

    bool txMakeBetVerify(const CTransaction& tx)
    {
        try
        {
            return txMakeBetVerify(tx, MAKE_MODULO_GAME_INDICATOR);
        }
        catch(...)
        {
            LogPrintf("modulo::txMakeBetVerify exception occured");
            return false;
        }

    };

}
