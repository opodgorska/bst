// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <validation.h>
#include <key_io.h>
#include "processunspent.h"

ProcessUnspent::ProcessUnspent(const CWalletRef pwallet, const std::vector<std::string>& addresses, bool include_unsafe, 
                               int nMinDepth, int nMaxDepth, CAmount nMinimumAmount, CAmount nMaximumAmount,
                               CAmount nMinimumSumAmount, uint64_t nMaximumCount) : entryArray(UniValue::VARR)
{
    std::set<CTxDestination> destinations;
    for (unsigned int idx = 0; idx < addresses.size(); idx++)
    {
        CTxDestination dest = DecodeDestination(addresses[idx]);
        if (!IsValidDestination(dest)) 
        {
            throw std::runtime_error(std::string("Invalid Bitcoin address: ") + addresses[idx]);
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

            if (scriptPubKey.IsPayToScriptHash()) 
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
}

ProcessUnspent::~ProcessUnspent() {}

bool ProcessUnspent::getUtxForAmount(UniValue& utx, double requiredAmount)
{
    bool isEnoughAmount=false;
    double amount=0.0;
    size_t size=entryArray.size();
    for(size_t i=0;i<size;++i)
    {
        amount+=entryArray[i][std::string("amount")].get_real();
        utx.push_back(entryArray[i]);
        if(amount>=requiredAmount)
        {
            isEnoughAmount=true;
            break;
        }
    }
    if(!isEnoughAmount)
    {
        utx.clear();
    }
    
    return isEnoughAmount;
}

std::string getChangeAddress(const CWalletRef pwallet, OutputType output_type)
{
    LOCK2(cs_main, pwallet->cs_wallet);

    if (!pwallet->IsLocked())
    {
        pwallet->TopUpKeyPool();
    }

    if (output_type != OUTPUT_TYPE_LEGACY &&
        output_type != OUTPUT_TYPE_P2SH_SEGWIT &&
        output_type != OUTPUT_TYPE_BECH32)
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
