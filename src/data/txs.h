// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXS_H
#define TXS_H

#include <consensus/params.h>
#include <wallet/wallet.h>
#include <univalue.h>
#include <data/datautils.h>

class Txs
{
    typedef std::vector<unsigned char> valtype;
public:
    Txs(CWallet* const pwallet, int64_t nLockTime, bool rbfOptIn, bool allowhighfees);
    Txs(int32_t txVersion, CWallet* const pwallet, int64_t nLockTime, bool rbfOptIn, bool allowhighfees);
    ~Txs();

    UniValue createTx(const UniValue& inputs, const UniValue& sendTo);
    UniValue signTx();
    UniValue sendTx();

protected:
    CMutableTransaction mtx;
    CWallet* const pwallet;
    int64_t nLockTime;
    bool rbfOptIn;
    bool allowhighfees;

private:
    virtual UniValue createTxImp(const UniValue& inputs, const UniValue& sendTo)=0;
    virtual UniValue signTxImp()=0;
    virtual UniValue sendTxImp();
    
protected:
    UniValue getnewaddress(CTxDestination& dest, OutputType output_type = OutputType::LEGACY);
    bool Sign1(const SigningProvider& provider, const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion);
    CScript PushAll(const std::vector<valtype>& values);
};

#endif
