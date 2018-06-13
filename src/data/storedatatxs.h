// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef STOREDATATXS_H
#define STOREDATATXS_H

#include <wallet/wallet.h>
#include <univalue.h>

class StoreDataTxs
{
public:
    StoreDataTxs(const CWalletRef pwallet, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~StoreDataTxs();

    UniValue createDataTx(const UniValue& inputs, const UniValue& sendTo);
    UniValue signTx();
    UniValue sendTx();

private:
    CMutableTransaction mtx;
    const CWalletRef pwallet;
    int64_t nLockTime;
    bool rbfOptIn;
    bool allowhighfees;
};

#endif
