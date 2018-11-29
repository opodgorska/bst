// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iomanip>
#include <amount.h>
#include <games/modulo/moduloutils.h>
#include <games/modulo/modulotxs.h>
#include <clocale>
#include <logging.h>

namespace modulo
{

    bool bet2Vector(const std::string& betTypePattern, std::vector<int>& bet)
    {
        int len;
        int* bet_;
        bool res=false;

        if( (betTypePattern.find_first_not_of( "0123456789" ) == std::string::npos) )
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern);
            }
            catch(...)
            {
                LogPrintf("bet2Vector: lottery number error: %d\n", betNum);
                return res;
            }

            straight = betNum;
            bet_ = const_cast<int* >(&straight);
            len = 1;
            res = true;
        }

        if(betTypePattern.find("straight_")==0 || betTypePattern.find("STRAIGHT_")==0)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("straight_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: stright number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>36)
            {
                LogPrintf("bet2Vector: stright number error: %d\n", betNum);
                return res;
            }
            straight = betNum;
            bet_ = const_cast<int* >(&straight);
            len = 1;
            res = true;
        }
        else if((betTypePattern.find("split_")==0 || betTypePattern.find("SPLIT_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("split_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: split number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>57)
            {
                LogPrintf("bet2Vector: split number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(split[betNum-1]);
            len = 2;
            res = true;
        }
        else if((betTypePattern.find("street_")==0 || betTypePattern.find("STREET_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("street_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: street number error: %d\n", betNum);
                return res;
            }        
            if(betNum<1 || betNum>12)
            {
                LogPrintf("bet2Vector: street number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(street[betNum-1]);
            len = 3;
            res = true;
        }
        else if((betTypePattern.find("corner_")==0 || betTypePattern.find("CORNER_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("corner_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: corner number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>22)
            {
                LogPrintf("bet2Vector: corner number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(corner[betNum-1]);
            len = 4;
            res = true;
        }
        else if((betTypePattern.find("line_")==0 || betTypePattern.find("LINE_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("line_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: line number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>11)
            {
                LogPrintf("bet2Vector: line number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(line[betNum-1]);
            len = 6;
            res = true;
        }
        else if((betTypePattern.find("column_")==0 || betTypePattern.find("COLUMN_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("column_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: column number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>3)
            {
                LogPrintf("bet2Vector: column number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(column[betNum-1]);
            len = 12;
            res = true;
        }
        else if((betTypePattern.find("dozen_")==0 || betTypePattern.find("DOZEN_")==0))
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("dozen_").length()));
            }
            catch(...)
            {
                LogPrintf("bet2Vector: dozen number error: %d\n", betNum);
                return res;
            }
            if(betNum<1 || betNum>3)
            {
                LogPrintf("bet2Vector: dozen number error: %d\n", betNum);
                return res;
            }
            bet_ = const_cast<int* >(dozen[betNum-1]);
            len = 12;
            res = true;
        }
        else if((betTypePattern.find("low")==0 || betTypePattern.find("LOW")==0))
        {
            bet_ = const_cast<int* >(low);
            len = 18;
            res = true;
        }
        else if((betTypePattern.find("high")==0 || betTypePattern.find("HIGH")==0))
        {
            bet_ = const_cast<int* >(high);
            len = 18;
            res = true;
        }
        else if((betTypePattern.find("even")==0 || betTypePattern.find("EVEN")==0))
        {
            bet_ = const_cast<int* >(even);
            len = 18;
            res = true;
        }
        else if((betTypePattern.find("odd")==0 || betTypePattern.find("ODD")==0))
        {
            bet_ = const_cast<int* >(odd);
            len = 18;
            res = true;
        }
        else if((betTypePattern.find("red")==0 || betTypePattern.find("RED")==0))
        {
            bet_ = const_cast<int* >(red);
            len = 18;
            res = true;
        }
        else if((betTypePattern.find("black")==0 || betTypePattern.find("BLACK")==0))
        {
            bet_ = const_cast<int* >(black);
            len = 18;
            res = true;
        }
        
        for(int i=0;i<len;++i)
        {
            bet.push_back(bet_[i]);
        }
        
        return res;
    }
    

    int getRouletteBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range)
    {
        std::setlocale(LC_NUMERIC, "C");
        std::setprecision(9);
        reward = 0;
        size_t at=betTypePattern.find("@");
        if(at==std::string::npos)
        {
            throw std::runtime_error(std::string("Incomplete bet type"));
        }
        size_t stringLen = betTypePattern.find("+", at);
        amount=std::stod(betTypePattern.substr(at+1, stringLen-at-1));
        betType=betTypePattern.substr(0, at);
        if(betTypePattern.find("straight_")==0 || betTypePattern.find("STRAIGHT_")==0)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("straight_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for straight type are 1, 2, ...")+std::to_string(range));
            }
            if(betNum<1 || betNum>range)
            {
                throw std::runtime_error(std::string("Possible numbers for straight type are 1, 2, ...")+std::to_string(range));
            }
            straight = betNum;
            bet = const_cast<int* >(&straight);
            len = 1;
            reward = range/len;
        }
        else if((betTypePattern.find("split_")==0 || betTypePattern.find("SPLIT_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("split_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for split type are 1, 2, ...57"));
            }
            if(betNum<1 || betNum>57)
            {
                throw std::runtime_error(std::string("Possible numbers for split type are 1, 2, ...57"));
            }
            bet = const_cast<int* >(split[betNum-1]);
            len = 2;
            reward = range/len;
        }
        else if((betTypePattern.find("street_")==0 || betTypePattern.find("STREET_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("street_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for street type are 1, 2, ...12"));
            }        
            if(betNum<1 || betNum>12)
            {
                throw std::runtime_error(std::string("Possible numbers for street type are 1, 2, ...12"));
            }
            bet = const_cast<int* >(street[betNum-1]);
            len = 3;
            reward = range/len;
        }
        else if((betTypePattern.find("corner_")==0 || betTypePattern.find("CORNER_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("corner_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for corner type are 1, 2, ...22"));
            }
            if(betNum<1 || betNum>22)
            {
                throw std::runtime_error(std::string("Possible numbers for corner type are 1, 2, ...22"));
            }
            bet = const_cast<int* >(corner[betNum-1]);
            len = 4;
            reward = range/len;
        }
        else if((betTypePattern.find("line_")==0 || betTypePattern.find("LINE_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("line_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for line type are 1, 2, ...11"));
            }
            if(betNum<1 || betNum>11)
            {
                throw std::runtime_error(std::string("Possible numbers for line type are 1, 2, ...11"));
            }
            bet = const_cast<int* >(line[betNum-1]);
            len = 6;
            reward = range/len;
        }
        else if((betTypePattern.find("column_")==0 || betTypePattern.find("COLUMN_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("column_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers for column type are 1, 2, 3"));
            }
            if(betNum<1 || betNum>3)
            {
                throw std::runtime_error(std::string("Possible numbers for column type are 1, 2, 3"));
            }
            bet = const_cast<int* >(column[betNum-1]);
            len = 12;
            reward = range/len;
        }
        else if((betTypePattern.find("dozen_")==0 || betTypePattern.find("DOZEN_")==0) && range==36)
        {
            int betNum;
            try
            {
                betNum=std::stoi(betTypePattern.substr(std::string("dozen_").length(), at));
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible dozen for line type are 1, 2, 3"));
            }
            if(betNum<1 || betNum>3)
            {
                throw std::runtime_error(std::string("Possible dozen for line type are 1, 2, 3"));
            }
            bet = const_cast<int* >(dozen[betNum-1]);
            len = 12;
            reward = range/len;
        }
        else if((betTypePattern.find("low")==0 || betTypePattern.find("LOW")==0) && range==36)
        {
            bet = const_cast<int* >(low);
            len = 18;
            reward = range/len;
        }
        else if((betTypePattern.find("high")==0 || betTypePattern.find("HIGH")==0) && range==36)
        {
            bet = const_cast<int* >(high);
            len = 18;
            reward = range/len;
        }
        else if((betTypePattern.find("even")==0 || betTypePattern.find("EVEN")==0) && range==36)
        {
            bet = const_cast<int* >(even);
            len = 18;
            reward = range/len;
        }
        else if((betTypePattern.find("odd")==0 || betTypePattern.find("ODD")==0) && range==36)
        {
            bet = const_cast<int* >(odd);
            len = 18;
            reward = range/len;
        }
        else if((betTypePattern.find("red")==0 || betTypePattern.find("RED")==0) && range==36)
        {
            bet = const_cast<int* >(red);
            len = 18;
            reward = range/len;
        }
        else if((betTypePattern.find("black")==0 || betTypePattern.find("BLACK")==0) && range==36)
        {
            bet = const_cast<int* >(black);
            len = 18;
            reward = range/len;
        }

        return stringLen;
    }

    int getModuloBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range)
    {
        std::setlocale(LC_NUMERIC, "C");
        std::setprecision(9);
        reward = 0;
        size_t at=betTypePattern.find("@");
        if(at==std::string::npos)
        {
            throw std::runtime_error(std::string("Incomplete bet type"));
        }
        size_t stringLen = betTypePattern.find("+", at);
        amount=std::stod(betTypePattern.substr(at+1, stringLen-at-1));
        betType=betTypePattern.substr(0, at);

        if( (betType.find_first_not_of( "0123456789" ) == std::string::npos) )
        {
            int betNum;
            try
            {
                betNum=std::stoi(betType);
            }
            catch(...)
            {
                throw std::runtime_error(std::string("Possible numbers are 1, 2, ...")+std::to_string(range));
            }
            if(betNum<1 || betNum>range)
            {
                throw std::runtime_error(std::string("Possible numbers are 1, 2, ...")+std::to_string(range));
            }
            straight = betNum;
            bet = const_cast<int* >(&straight);
            len = 1;
            reward = range/len;
        }
        
        return stringLen;
    }

    void parseBetType(std::string& betTypePattern, int range, std::vector<double>& betAmounts, std::vector<std::string>& betTypes, std::vector<std::vector<int> >& betArrays, bool isRoulette)
    {
        for(int i=0;true;++i)
        {
            int reward;
            double betAmount;
            int* betArray;
            int betLen;
            std::string betType;
            size_t len;

            if(isRoulette)
            {
                len=getRouletteBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
            }
            else
            {
                len=getModuloBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
            }
            
            if(reward <= 0 || reward > MAX_REWARD)
            {
                throw std::runtime_error(std::string("Incorrect type of bet"));
            }

            if(betAmount <= 0)
            {
                throw std::runtime_error(std::string("Amount must be greater than 0"));
            }

            if(static_cast<CAmount>(betAmount*COIN)*static_cast<CAmount>(reward) > MAX_PAYOFF)
            {
                throw std::runtime_error(std::string("Reward exceedes the limit of: ")+std::to_string(MAX_PAYOFF/COIN)+std::string(" BST"));
            }

            std::vector<int> betVec;
            betArrays.push_back(betVec);
            std::copy(&betArray[0], &betArray[betLen], back_inserter(betArrays[i]));
            betAmounts.push_back(betAmount);
            betTypes.push_back(betType);
            
            if(len==std::string::npos)
            {
                break;
            }
            betTypePattern=betTypePattern.substr(len+1);
        }
    }
}
