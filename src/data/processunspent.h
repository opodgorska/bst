// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROCESSUNSPENT_H
#define PROCESSUNSPENT_H

#include <wallet/wallet.h>
#include <univalue.h>

class ProcessUnspent
{
public:
	ProcessUnspent(CWallet* const pwallet, const std::vector<std::string>& addresses, bool include_unsafe = true, 
                    int nMinDepth = 0, int nMaxDepth = 9999999, CAmount nMinimumAmount = 0, CAmount nMaximumAmount = MAX_MONEY,
                    CAmount nMinimumSumAmount = MAX_MONEY, uint64_t nMaximumCount = 0);
    ~ProcessUnspent();
    bool getUtxForAmount(UniValue& utx, const CFeeRate& feeRate, size_t dataSize, double amount, double& fee);
	
private:
    CWallet* const wallet;
    std::vector<UniValue> entryArray;
};

double computeFee(const CWallet& wallet, size_t dataSize);
std::string getChangeAddress(CWallet* const pwallet, OutputType type=OutputType::P2SH_SEGWIT);

#endif
