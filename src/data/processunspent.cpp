// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <core_io.h>
#include <validation.h>
#include <key_io.h>
#include <wallet/fees.h>
#include <wallet/coincontrol.h>
#include "processunspent.h"

ProcessUnspent::ProcessUnspent(CWallet* const pwallet, const std::vector<std::string>& addresses, bool include_unsafe, 
                               int nMinDepth, int nMaxDepth, CAmount nMinimumAmount, CAmount nMaximumAmount,
                               CAmount nMinimumSumAmount, uint64_t nMaximumCount) : wallet(pwallet)
{
    std::set<CTxDestination> destinations;
    for (unsigned int idx = 0; idx < addresses.size(); idx++)
    {
        CTxDestination dest = DecodeDestination(addresses[idx]);
        if (!IsValidDestination(dest)) 
        {
            throw std::runtime_error(std::string("Invalid BST address: ") + addresses[idx]);
        }
        if (!destinations.insert(dest).second) 
        {
            throw std::runtime_error(std::string("Invalid parameter, duplicated address: ") + addresses[idx]);
        }
    }

    // Make sure the results are valid at least up to the most recent block
    pwallet->BlockUntilSyncedToCurrentChain();

    std::vector<COutput> vecOutputs;
    LOCK2(cs_main, pwallet->cs_wallet);

    pwallet->AvailableCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth);
    for (const COutput& out : vecOutputs)
    {
        CTxDestination address;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);

        if (destinations.size() && (!fValidAddress || !destinations.count(address)))
        {
            continue;
        }

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", out.tx->GetHash().GetHex());
        entry.pushKV("vout", out.i);

        if (fValidAddress) 
        {
            entry.pushKV("address", EncodeDestination(address));

            if (pwallet->mapAddressBook.count(address)) 
            {
                entry.pushKV("account", pwallet->mapAddressBook[address].name);
            }

            if (scriptPubKey.IsPayToScriptHash(true))
            {
                const CScriptID& hash = boost::get<CScriptID>(address);
                CScript redeemScript;
                if (pwallet->GetCScript(hash, redeemScript)) 
                {
                    entry.pushKV("redeemScript", HexStr(redeemScript.begin(), redeemScript.end()));
                }
            }
        }

        entry.pushKV("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end()));
        entry.pushKV("amount", ValueFromAmount(out.tx->tx->vout[out.i].nValue));
        entry.pushKV("confirmations", out.nDepth);
        entry.pushKV("spendable", out.fSpendable);
        entry.pushKV("solvable", out.fSolvable);
        entry.pushKV("safe", out.fSafe);
        entryArray.push_back(entry);
    }
    std::sort(entryArray.begin(), entryArray.end(),
    [](const UniValue& a, const UniValue& b)
    {
        return a["confirmations"].get_int()> b["confirmations"].get_int();
    });
}

ProcessUnspent::~ProcessUnspent() {}

bool ProcessUnspent::getUtxForAmount(UniValue& utx, const CFeeRate& feeRate, size_t dataSize, double amount, double& fee)
{
    bool isEnoughAmount=false;
    double amountAvailable=0.0;
    
    fee=static_cast<double>(feeRate.GetFee(dataSize))/COIN;

    size_t size=entryArray.size();
    for(size_t i=0;i<size;++i)
    {
        double requiredAmount=amount+fee;
        amountAvailable+=entryArray[i][std::string("amount")].get_real();
        utx.push_back(entryArray[i]);
        if(amountAvailable>=requiredAmount)
        {
            isEnoughAmount=true;
            break;
        }

        //we must increase transaction size by adding another input, therefore fee is increased as well
        constexpr size_t txInputSize=145;
        dataSize+=txInputSize;
        fee=static_cast<double>(feeRate.GetFee(dataSize))/COIN;
    }
    if(!isEnoughAmount)
    {
        utx.clear();
    }
    
    return isEnoughAmount;
}

double computeFee(const CWallet& wallet, size_t dataSize)
{
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf=true;
    FeeCalculation feeCalc;
    CFeeRate nFeeRateNeeded = GetMinimumFeeRate(wallet, coin_control, ::mempool, ::feeEstimator, &feeCalc);
    return static_cast<double>(nFeeRateNeeded.GetFee(dataSize))/COIN;
}

std::string getChangeAddress(CWallet* const pwallet, OutputType output_type)
{
    LOCK2(cs_main, pwallet->cs_wallet);

    if (!pwallet->IsLocked())
    {
        pwallet->TopUpKeyPool();
    }

    if (output_type != OutputType::LEGACY &&
        output_type != OutputType::P2SH_SEGWIT &&
        output_type != OutputType::BECH32)
    {
        throw std::runtime_error(std::string("Unknown address type"));
    }

    CReserveKey reservekey(pwallet);
    CPubKey vchPubKey;
    if (!reservekey.GetReservedKey(vchPubKey, true))
    {
        throw std::runtime_error(std::string("Error: Keypool ran out, please call keypoolrefill first"));
    }

    reservekey.KeepKey() ;

    pwallet->LearnRelatedScripts(vchPubKey, output_type);
    CTxDestination dest = GetDestinationForKey(vchPubKey, output_type);

    return EncodeDestination(dest);
}
