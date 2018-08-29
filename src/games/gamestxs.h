// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMETXS_H
#define GAMETXS_H

#include <consensus/params.h>
#include <wallet/wallet.h>
#include <univalue.h>
#include <data/datautils.h>
#include <data/txs.h>
#include <games/gamesutils.h>

class MakeBetTxs : public Txs
{
public:
    MakeBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, int32_t txVersion, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~MakeBetTxs();

    UniValue getTx();
    static bool checkBetRewardSum(double& rewardAcc, const CTransaction& tx, const Consensus::Params& params);

private:
    bool isNewAddrGenerated;
    CTxDestination dest;
    CScript redeemScript;

private:
    UniValue createTxImp(const UniValue& inputs, const UniValue& sendTo) override;
    UniValue signTxImp() override;
};


class GetBetTxs : public Txs
{
    typedef std::vector<unsigned char> valtype;
public:
    GetBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, const UniValue& prevTxBlockHash, ArgumentOperation* operation, 
              int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~GetBetTxs();
    
    UniValue getTx();

private:
    UniValue prevTxBlockHash;
    ArgumentOperation* operation;

private:
    UniValue createTxImp(const UniValue& inputs, const UniValue& sendTo) override;
    UniValue signTxImp() override;

    UniValue SignRedeemBetTransaction(const UniValue hashType);
    bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);
    unsigned int getMakeTxBlockHash(unsigned int argument);
};

size_t getRedeemScriptSize(const CScript& redeemScript);
CScript getRedeemScript(CWallet* const pwallet, const std::string& scriptPubKeyStr);
int getReward(CWallet* const pwallet, const std::string& scriptPubKeyStr);
unsigned int getArgument(const CScript& redeemScript);

#endif
