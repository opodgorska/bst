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

//bioinfo hardfork due to roulette bets definition change
#define ROULETTE_NEW_DEFS (108600)
//bioinfo hardfork due to incorrect format of makebet transactions
#define MAKEBET_FORMAT_VERIFY (139378)
//bioinfo hardfork due to block transactions potential reward over limit
#define MAKEBET_REWARD_LIMIT (156520)

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

bool txVerify(int nSpendHeight, const CTransaction& tx, CAmount in, CAmount out, CAmount& fee, ArgumentOperation* operation, GetReward* getReward, CompareBet2Vector* compareBet2Vector, int32_t indicator, CAmount maxPayoff, int32_t maxReward);

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
    bool checkPotentialRewardLimit(CAmount &rewardSum, const CTransaction& txn);

private:
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

class VerifyMakeBetFormat
{
public:
    VerifyMakeBetFormat(GetReward *getReward, int32_t makeBetIndicator, CAmount maxReward);
    bool txMakeBetVerify(const CTransaction& tx);
private:
    GetReward* m_getReward;
    int32_t m_indicator;
    const CAmount m_maxReward;
};

bool isMakeBetTx(const CTransaction& tx, int32_t makeBetIndicator);
std::string getBetType(const CTransaction& tx);
unsigned int getArgument(std::string& betType);

#endif
