// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMESUTILS_H
#define GAMESUTILS_H

#include <consensus/params.h>
#include <univalue.h>
#include <outputtype.h>

UniValue findTx(const std::string& txid);

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

CKeyID getTxKeyID(const CTransaction& tx, int inputIdx=0);
CScript createScriptPubkey(const CTransaction& prevTx);

class MakeBetWinningProcess
{
public:
    MakeBetWinningProcess(const CTransaction& tx, uint256 hash);
    bool isMakeBetWinning();
    CAmount getMakeBetPayoff();
};

#endif
