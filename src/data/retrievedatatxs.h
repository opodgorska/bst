// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RETRIEVEDATATXS_H
#define RETRIEVEDATATXS_H


#include <univalue.h>

class RetrieveDataTxs
{
public:
    RetrieveDataTxs(const std::string& txid, const std::string& blockHash = "");
    ~RetrieveDataTxs();
    std::string getTxData();

private:
    UniValue transaction;
    int hexStr2int(const std::string& str);
    void reverseEndianess(std::string& str);
};

#endif
