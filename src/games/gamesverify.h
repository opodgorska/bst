// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMEVERIFY_H
#define GAMEVERIFY_H

#include <consensus/params.h>
#include <wallet/wallet.h>
#include <games/gamesutils.h>

class GetReward
{
public:
    virtual int operator()(const std::string& betType, unsigned int argument)=0;
};

bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee, ArgumentOperation* operation, GetReward* getReward, int32_t indicator, CAmount maxPayoff, int32_t maxReward);

#endif
