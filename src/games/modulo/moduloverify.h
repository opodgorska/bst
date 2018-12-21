// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MODULOVERIFY_H
#define MODULOVERIFY_H

#include <games/gamesverify.h>

namespace modulo
{

    class GetModuloReward : public GetReward
    {
    public:
        virtual int operator()(const std::string& betType, unsigned int modulo) override;
    };

    class VerifyMakeModuloBetTx : public VerifyMakeBetTx
    {
    public:
        virtual bool isWinning(const std::string& betType, unsigned int maxArgument, unsigned int argument) override;
    };

    class CompareModuloBet2Vector : public CompareBet2Vector
    {
    public:
        virtual bool operator()(int nSpendHeight, const std::string& betTypePattern, const std::vector<int>& betNumbers) override;
    };

    bool isMakeBetTx(const CTransaction& tx);
    bool txVerify(int nSpendHeight, const CTransaction& tx, CAmount in, CAmount out, CAmount& fee);
    bool isBetPayoffExceeded(const Consensus::Params& params, const CBlock& block);
    bool txMakeBetVerify(const CTransaction& tx, bool ignoreHardfork = false);
    bool checkBetsPotentialReward(CAmount& rewardSum, CAmount &betsSum, const CTransaction& txn, bool ignoreHardfork = false);
    CAmount getSumOfTxnBets(const CTransaction& txn);
}

#endif
