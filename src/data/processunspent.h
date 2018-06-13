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
	ProcessUnspent(const CWalletRef pwallet, const std::vector<std::string>& addresses, bool include_unsafe = true, 
				   int nMinDepth = 1, int nMaxDepth = 9999999, CAmount nMinimumAmount = 0, CAmount nMaximumAmount = MAX_MONEY,
				   CAmount nMinimumSumAmount = MAX_MONEY, uint64_t nMaximumCount = 0);
	~ProcessUnspent();
	bool getUtxForAmount(UniValue& utx, double requiredAmount);
	
private:
	UniValue entryArray;
};

std::string getChangeAddress(const CWalletRef pwallet, OutputType type=OUTPUT_TYPE_LEGACY);

#endif
