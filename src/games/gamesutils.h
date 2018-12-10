// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMESUTILS_H
#define GAMESUTILS_H

#include <consensus/params.h>
#include <univalue.h>
#include <outputtype.h>

UniValue findTx(const std::string& txid);
std::tuple<UniValue, CTransactionRef> findTxData(const std::string& txid);
std::tuple<std::string, size_t> getBetData(const UniValue& txPrev);
unsigned int blockHashStr2Int(const std::string& hashStr);
unsigned int getArgumentFromBetType(std::string& betType, uint max_limit = std::numeric_limits<uint>::max());

const std::string OP_RETURN_NOT_FOUND = "OP_RETURN not found";
const std::string LENGTH_TOO_LARGE = "betType length is too-large";
const std::string INPUT_NOT_FOUND_OR_SPENT = "Input not found or already spent";
const std::string FAILED_OP_EQUALVERIFY_OPERATION = "Script failed an OP_EQUALVERIFY operation";


class ArgumentOperation
{
public:
    ArgumentOperation();
    ArgumentOperation(unsigned int argument);
    void setArgument(unsigned int argument);
    virtual unsigned int operator()(unsigned int blockHash) = 0;

protected:
    unsigned int argument;
};

#endif
