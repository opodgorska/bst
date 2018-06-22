// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOTTERY_H
#define LOTTERY_H

#include <consensus/params.h>
#include <wallet/wallet.h>
#include <univalue.h>
#include <data/datautils.h>

class MakeBetTxs
{
public:
    static constexpr double ACCUMULATED_BET_REWARD_FOR_BLOCK=0.5;

    MakeBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false, int32_t txVersion=(MAKE_BET_INDICATOR | CTransaction::CURRENT_VERSION));
    ~MakeBetTxs();

    UniValue createTx(const UniValue& inputs, const UniValue& sendTo);
    UniValue signTx();
    UniValue sendTx();

    UniValue getTx();
    UniValue getRedeemScriptAsm();
    UniValue getRedeemScriptHex();
    static bool checkBetRewardSum(double& rewardAcc, const CTransaction& tx, const Consensus::Params& params);

private:
    CMutableTransaction mtx;
    CWallet* const pwallet;
    int64_t nLockTime;
    bool rbfOptIn;
    bool allowhighfees;
    CScript redeemScript;
};


class GetBetTxs
{
public:
    GetBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, const UniValue& prevTxBlockHash, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~GetBetTxs();

    UniValue createTx(const UniValue& inputs, const UniValue& sendTo);
    UniValue signTx();
    UniValue sendTx();
    
    UniValue getTx();
    static UniValue findTx(const std::string& txid);
    static bool txVerify(const CTransaction& tx, CAmount in, CAmount out);

private:
    CMutableTransaction mtx;
    CWallet* const pwallet;
    UniValue prevTxBlockHash;
    int64_t nLockTime;
    bool rbfOptIn;
    bool allowhighfees;
    
    UniValue SignRedeemBetTransaction(const UniValue hashType);
    bool ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata);
};

template<typename T>
T getMask(T in)
{
    if(in==0)
    {
        return 1;
    }

    T div=1;
    size_t i;
    for(i=0;i<sizeof(in)*8;++i)
    {
        if((in/div)==0)
        {
            break;
        }
        div<<=1;
    }

   i--;
   T mask=1;
   for(;i>0;--i)
   {
        mask<<=1;
        mask|=1;
   }

   return mask;
}

template<typename T>
T maskToReward(T in)
{
    T div=1;
    size_t i;
    for(i=0;i<sizeof(in)*8;++i)
    {
        if((in/div)==0)
        {
            break;
        }
        div<<=1;
    }

    return 1<<i;
}

template<typename T>
T getReward(CWallet* const pwallet, const std::string& scriptPubKeyStr)
{
    std::vector<unsigned char> scriptPubKeyChr(scriptPubKeyStr.length()/2, 0);
    hex2bin(scriptPubKeyChr, scriptPubKeyStr);
    CScript scriptPubKey(scriptPubKeyChr.begin(), scriptPubKeyChr.end());
    //std::cout<<"scriptPubKey: "<<HexStr(scriptPubKeyChr.begin(), scriptPubKeyChr.end())<<std::endl;
    CScript redeemScript;
    std::vector<unsigned char> scriptPubKeyHashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
    pwallet->GetCScript(uint160(scriptPubKeyHashBytes), redeemScript);//get redeemScript by its hash
    //std::cout<<"redeemScript: "<<HexStr(redeemScript.begin(), redeemScript.end())<<std::endl;
    
    std::vector<unsigned char> mask_(redeemScript.end()-36, redeemScript.end()-32);
    T mask=0;
    array2type(mask_, mask);

    return maskToReward(mask);
}

#endif
