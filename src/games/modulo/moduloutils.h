// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MODULOUTILS_H
#define MODULOUTILS_H

#include <string>
#include <vector>

namespace modulo
{

    static int straight;

    static size_t const splitBetsNum=57;
    static size_t const streetBetsNum=12;
    static size_t const cornerBetsNum=22;
    static size_t const lineBetsNum=11;
    static size_t const columnBetsNum=3;
    static size_t const dozenBetsNum=3;

    static int const split[57][2]=
    {
        {1, 4},
        {4, 7},
        {7, 10},
        {10, 13},
        {13, 16},
        {16, 19},
        {19, 22},
        {22, 25},
        {25, 28},
        {28, 31},
        {31, 34},
        {2, 5},
        {5, 8},
        {8, 11},
        {11, 14},
        {14, 17},
        {17, 20},
        {20, 23},
        {23, 26},
        {26, 29},
        {29, 32},
        {32, 35},
        {3, 6},
        {6, 9},
        {9, 12},
        {12, 15},
        {15, 18},
        {18, 21},
        {21, 24},
        {24, 27},
        {27, 30},
        {30, 33},
        {33, 36},
        {1, 2},
        {2, 3},
        {4, 5},
        {5, 6},
        {7, 8},
        {8, 9},
        {10, 11},
        {11, 12},
        {13, 14},
        {14, 15},
        {16, 17},
        {17, 18},
        {19, 20},
        {20, 21},
        {22, 23},
        {23, 24},
        {25, 26},
        {26, 27},
        {28, 29},
        {29, 30},
        {31, 32},
        {32, 33},
        {34, 35},
        {35, 36}
    };

    static int const street[12][3]=
    {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9},
        {10, 11, 12},
        {13, 14, 15},
        {16, 17, 18},
        {19, 20, 21},
        {22, 23, 24},
        {25, 26, 27},
        {28, 29, 30},
        {31, 32, 33},
        {34, 35, 36}
    };

    static int const corner[22][4]=
    {
        {1, 2, 4, 5},
        {4, 5, 7, 8},
        {7, 8, 10, 11},
        {10, 11, 13, 14},
        {13, 14, 16, 17},
        {16, 17, 19, 20},
        {19, 20, 22, 23},
        {22, 23, 25, 26},
        {25, 26, 28, 29},
        {28, 29, 31, 32},
        {31, 32, 34, 35},
        {2, 3, 5, 6},
        {5, 6, 8, 9},
        {8, 9, 11, 12},
        {11, 12, 14, 15},
        {14, 15, 17, 18},
        {17, 18, 20, 21},
        {20, 21, 23, 24},
        {23, 24, 26, 27},
        {26, 27, 29, 30},
        {29, 30, 32, 33},
        {32, 33, 35, 36}
    };

    static int const line[11][6]=
    {
        {1, 2, 3, 4, 5, 6},
        {4, 5, 6, 7, 8, 9},
        {7, 8, 9, 10, 11, 12},
        {10, 11, 12, 13, 14, 15},
        {13, 14, 15, 16, 17, 18},
        {16, 17, 18, 19, 20, 21},
        {19, 20, 21, 22, 23, 24},
        {22, 23, 24, 25, 26, 27},
        {25, 26, 27, 28, 29, 30},
        {28, 29, 30, 31, 32, 33},
        {31, 32, 33, 34, 35, 36}
    };

    static int const column[3][12]=
    {
        {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34},
        {2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35},
        {3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36}
    };

    static int const dozen[3][12]=
    {
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
        {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}
    };

    static int const low[18]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
    static int const high[18]={19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

    static int const even[18]={2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36};
    static int const odd[18]={1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35};

    static int const red[18]={1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36};
    static int const black[18]={2, 4, 6, 8, 10, 11, 13, 15, 17, 20, 22, 24, 26, 28, 29, 31, 33, 35};

    int getRouletteBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range);
    int getModuloBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range);
    void parseBetType(std::string& betTypePattern, int range, std::vector<double>& betAmounts, std::vector<std::string>& betTypes, std::vector<std::vector<int> >& betArrays, bool isRoulette);
    void parseTxIds(const std::string& txidsPattern, std::vector<std::string>& txids);
    bool bet2Vector(const std::string& betTypePattern, std::vector<int>& bet);
}

#endif
