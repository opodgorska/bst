// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <games/gamesverify.h>
#include <games/modulo/moduloverify.h>
#include <games/modulo/modulotxs.h>

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

    bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee)
    {
        ModuloOperation moduloOperation;
        GetModuloReward getModuloReward;
        return txVerify(tx, in, out, fee, &moduloOperation, &getModuloReward, MAKE_MODULO_GAME_INDICATOR, MAX_PAYOFF, MAX_REWARD);
    };

}
