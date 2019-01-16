// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMEVERIFY_H
#define GAMEVERIFY_H

#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <wallet/wallet.h>
#include <games/gamesutils.h>
#include <data/datautils.h>

//bioinfo hardfork due to roulette bets definition change
#define ROULETTE_NEW_DEFS (108600)
//bioinfo hardfork due to incorrect format of makebet transactions
#define MAKEBET_FORMAT_VERIFY (224841)
//bioinfo hardfork due to incorrect getbet verification
#define GETBET_NEW_VERIFY (169757)

class GetReward
{
public:
    virtual int operator()(const std::string& betType, unsigned int argument)=0;
};

class CompareBet2Vector
{
public:
    virtual bool operator()(int nSpendHeight, const std::string& betTypePattern, const std::vector<int>& betNumbers)=0;
};

class VerifyMakeBetTx
{
public:
    virtual bool isWinning(const std::string& betType, unsigned int maxArgument, unsigned int argument)=0;
};

bool isBetTx(const CTransaction& tx, int32_t makeBetIndicator);

#endif
