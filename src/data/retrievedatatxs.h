// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RETRIEVEDATATXS_H
#define RETRIEVEDATATXS_H

#include <wallet/wallet.h>
#include <univalue.h>

class RetrieveDataTxs
{
public:
    RetrieveDataTxs(const std::string& txid, CWallet* const pwallet=nullptr, const std::string& blockHash = "");
    ~RetrieveDataTxs();
    std::vector<char> getTxData();

private:
    CTransactionRef tx;
};

#endif
