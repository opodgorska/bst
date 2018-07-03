// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <future>
#include <key_io.h>
#include <net.h>
#include <policy/rbf.h>
#include <rpc/rawtransaction.h>
#include <rpc/server.h>
#include <validation.h>
#include "storedatatxs.h"

StoreDataTxs::StoreDataTxs(CWallet* const pwallet_, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_) 
                          : Txs(pwallet_, nLockTime_, rbfOptIn_, allowhighfees_)
{
    createTxImp(inputs, sendTo);
}

StoreDataTxs::~StoreDataTxs() {}

UniValue StoreDataTxs::createTxImp(const UniValue& inputs, const UniValue& sendTo)
{
    CMutableTransaction& rawTx=mtx;

    if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
    {
        throw std::runtime_error(std::string("Invalid parameter, locktime out of range"));
    }
    rawTx.nLockTime = nLockTime;

    
    for (unsigned int idx = 0; idx < inputs.size(); idx++)
    {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
        {
            throw std::runtime_error(std::string("Invalid parameter, missing vout key"));
        }
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
        {
            throw std::runtime_error(std::string("Invalid parameter, vout must be positive"));
        }

        uint32_t nSequence;
        if (rbfOptIn) 
        {
            nSequence = MAX_BIP125_RBF_SEQUENCE;
        } 
        else if (rawTx.nLockTime) 
        {
            nSequence = std::numeric_limits<uint32_t>::max() - 1;
        } 
        else 
        {
            nSequence = std::numeric_limits<uint32_t>::max();
        }

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) 
        {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max()) 
            {
                throw std::runtime_error(std::string("Invalid parameter, sequence number is out of range"));
            } 
            else 
            {
                nSequence = (uint32_t)seqNr64;
            }
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }

    std::set<CTxDestination> destinations;
    std::vector<std::string> addrList = sendTo.getKeys();
    for (const std::string& name_ : addrList) 
    {

        if (name_ == "data") 
        {
            std::vector<unsigned char> data = ParseHexV(sendTo[name_].getValStr(),"Data");

            CTxOut out(0, CScript() << OP_RETURN << data);
            rawTx.vout.push_back(out);
        } 
        else 
        {
            CTxDestination destination = DecodeDestination(name_);
            if (!IsValidDestination(destination)) 
            {
                throw std::runtime_error(std::string("Invalid Bitcoin address: ") + name_);
            }

            if (!destinations.insert(destination).second) 
            {
                throw std::runtime_error(std::string("Invalid parameter, duplicated address: ") + name_);
            }

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = AmountFromValue(sendTo[name_]);

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }
    }

    if (rbfOptIn != SignalsOptInRBF(rawTx)) 
    {
        throw std::runtime_error(std::string("Invalid parameter combination: Sequence number(s) contradict replaceable option"));
    }
    
    return EncodeHexTx(rawTx);
}

UniValue StoreDataTxs::signTxImp()
{
    // Sign the transaction
    UniValue prevTxsUnival;
    prevTxsUnival.setNull();
    const UniValue hashType(std::string("ALL"));

    LOCK2(cs_main, pwallet->cs_wallet);
    return SignTransaction(mtx, prevTxsUnival, pwallet, false, hashType);
}
