// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STOREDATATXS_H
#define STOREDATATXS_H

#include <wallet/wallet.h>
#include <univalue.h>
#include <data/txs.h>

class StoreDataTxs : public Txs
{
public:
    StoreDataTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~StoreDataTxs();

private:
    UniValue createTxImp(const UniValue& inputs, const UniValue& sendTo) override;
    UniValue signTxImp() override;
};

#endif
