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

class GetReward
{
public:
    virtual int operator()(const std::string& betType, unsigned int argument)=0;
};

bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee, ArgumentOperation* operation, GetReward* getReward, int32_t indicator, CAmount maxPayoff, int32_t maxReward);

class VerifyMakeBetTx
{
public:
    virtual bool isWinning(const std::string& betType, unsigned int maxArgument, unsigned int argument)=0;
};

class VerifyBlockReward
{
public:
    VerifyBlockReward(const Consensus::Params& params, const CBlock& block_, ArgumentOperation* argumentOperation, GetReward* getReward, VerifyMakeBetTx* verifyMakeBetTx, int32_t makeBetIndicator, CAmount maxPayoff);
    bool isBetPayoffExceeded();

private:
    std::string getBetType(const CTransaction& tx);
    unsigned int getArgument(std::string& betType);
    bool isMakeBetTx(const CTransaction& tx);

private:
    const CBlock& block;
    ArgumentOperation* argumentOperation;
    GetReward* getReward;
    VerifyMakeBetTx* verifyMakeBetTx;
    unsigned int argumentResult;
    unsigned int blockHash;
    int32_t makeBetIndicator;
    CAmount blockSubsidy;
    const CAmount maxPayoff;
};

bool isMakeBetTx(const CTransaction& tx, int32_t makeBetIndicator);

#endif
