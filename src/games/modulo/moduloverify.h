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

    namespace ver_1
    {
        class VerifyBlockReward
        {
        public:
            VerifyBlockReward(const Consensus::Params& params, const CBlock& block_, 
                              ArgumentOperation* argumentOperation, GetReward* getReward, VerifyMakeBetTx* verifyMakeBetTx,
                              int32_t makeBetIndicator, CAmount maxPayoff);
            bool isBetPayoffExceeded();

        private:
            unsigned int getArgument(std::string& betType);

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
        
        bool isMakeBetTx(const CTransaction& tx);
        bool isInputBet(const CTxIn& input);
        bool txGetBetVerify(int nSpendHeight, const CTransaction& tx, CAmount in, CAmount out, CAmount& fee);
        bool isBetPayoffExceeded(const Consensus::Params& params, const CBlock& block);
        bool txMakeBetVerify(const CTransaction& tx);
    };
    
    namespace ver_2
    {
        class MakeBetWinningProcess
        {
        public:
            MakeBetWinningProcess(const CTransaction& tx, uint256 hash);
            bool isMakeBetWinning();
            CAmount getMakeBetPayoff();
        private:
            static const CAmount MAX_CAMOUNT = std::numeric_limits<CAmount>::max();
            const CTransaction& m_tx;
            uint256 m_hash;
            CAmount m_payoff=0;
        };

        bool isMakeBetTx(const CTransaction& tx);
        bool isGetBetTx(const CTransaction& tx);
        bool txGetBetVerify(const uint256& hashPrevBlock, const CBlock& currentBlock, const Consensus::Params& params, CAmount& fee);
        bool txMakeBetVerify(const CTransaction& tx);
    };
    
}



#endif
