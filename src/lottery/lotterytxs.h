// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOTTERYTXS_H
#define LOTTERYTXS_H

#include <consensus/params.h>
#include <wallet/wallet.h>
#include <univalue.h>
#include <data/datautils.h>
#include <data/txs.h>

static constexpr int MAX_BET_REWARD_POW=10;
static constexpr int MAX_BET_REWARD=(0x1<<MAX_BET_REWARD_POW);
static constexpr double ACCUMULATED_BET_REWARD_FOR_BLOCK=0.9;

class MakeBetTxs : public Txs
{
public:
    MakeBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false, int32_t txVersion=(MAKE_BET_INDICATOR | CTransaction::CURRENT_VERSION));
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

    UniValue getnewaddress(CTxDestination& dest, OutputType output_type = OutputType::LEGACY);
    void getOpReturnAccReward(double& rewardAcc, const CTransaction& tx, const UniValue& amount);
};


class GetBetTxs : public Txs
{
    typedef std::vector<unsigned char> valtype;
public:
    GetBetTxs(CWallet* const pwallet, const UniValue& inputs, const UniValue& sendTo, const UniValue& prevTxBlockHash, int64_t nLockTime=0, bool rbfOptIn=false, bool allowhighfees=false);
    ~GetBetTxs();
    
    UniValue getTx();
    static UniValue findTx(const std::string& txid);
    static bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee);

private:
    UniValue prevTxBlockHash;

private:
    UniValue createTxImp(const UniValue& inputs, const UniValue& sendTo) override;
    UniValue signTxImp() override;

    UniValue SignRedeemBetTransaction(const UniValue hashType);
    bool Sign1(const SigningProvider& provider, const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion);
    CScript PushAll(const std::vector<valtype>& values);
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
    //LogPrintf("getReward: scriptPubKey: %s\n", HexStr(scriptPubKeyChr.begin(), scriptPubKeyChr.end()));
    CScript redeemScript;
    std::vector<unsigned char> scriptPubKeyHashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
    pwallet->GetCScript(uint160(scriptPubKeyHashBytes), redeemScript);//get redeemScript by its hash
    //LogPrintf("getReward: redeemScript: %s\n", HexStr(redeemScript.begin(), redeemScript.end()));
    std::vector<unsigned char> mask_(redeemScript.end()-36, redeemScript.end()-32);
    T mask=0;
    array2type(mask_, mask);

    return maskToReward(mask);
}

#endif
