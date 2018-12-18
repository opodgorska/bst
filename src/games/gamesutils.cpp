// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <logging.h>
#include <policy/policy.h>
#include <rpc/server.h>
#include <uint256.h>
#include <univalue.h>
#include <validation.h>

#include <games/gamesutils.h>

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);


ArgumentOperation::ArgumentOperation() : argument(0)
{
};

ArgumentOperation::ArgumentOperation(unsigned int argument_) : argument(argument_)
{
};

void ArgumentOperation::setArgument(unsigned int argument_)
{
    argument=argument_;
};

UniValue findTx(const std::string& txid)
{
    LOCK(cs_main);

    bool in_active_chain = true;
    uint256 hash = ParseHashV(txid, "parameter 1");
    CBlockIndex* blockindex = nullptr;
    CTransactionRef tx;
    uint256 hash_block;
    UniValue result(UniValue::VOBJ);

    int i;
    for(i=chainActive.Height();i>=0;--i)
    {
        blockindex = chainActive[i];

        if (hash == Params().GenesisBlock().hashMerkleRoot)
        {
            // Special exception for the genesis block coinbase transaction
            throw std::runtime_error(std::string("The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved"));
        }

        if (GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex)) 
        {
            if(blockindex)
            {
                result.pushKV("in_active_chain", in_active_chain);
            }
            TxToJSON(*tx, hash_block, result);
            result.pushKV("blockhash", hash_block.ToString());
            break;
        }
    }

    if(i<0)
    {
        throw std::runtime_error(std::string("Transaction not in blockchain"));
    }

    return result;
}

CKeyID getTxKeyID(const CTransaction& tx, int inputIdx)
{
    if(!tx.vin[inputIdx].scriptWitness.IsNull())
    {
        return CKeyID(Hash160(tx.vin[inputIdx].scriptWitness.stack[1].begin(), tx.vin[inputIdx].scriptWitness.stack[1].end()));
    }

    return CKeyID(Hash160(tx.vin[inputIdx].scriptSig.begin(), tx.vin[inputIdx].scriptSig.end()));
}

CScript createScriptPubkey(const CTransaction& prevTx)
{
    const CKeyID keyID = getTxKeyID(prevTx);
    return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
}

MakeBetWinningProcess::MakeBetWinningProcess(const CTransaction& tx, uint256 hash)
{}

bool MakeBetWinningProcess::isMakeBetWinning()
{
    return true;
}

CAmount MakeBetWinningProcess::getMakeBetPayoff()
{
    return 0;
}

CAmount applyFee(CMutableTransaction& tx, int64_t nTxWeight, int64_t sigOpCost)
{
    if(tx.vout.size()==0)
    {
        return 0;
    }

    //compute fee for this transaction and equally subtract it from each output
    int64_t virtualSize = GetVirtualTransactionSize(nTxWeight, sigOpCost);
    CAmount totalFee = ::minRelayTxFee.GetFee(virtualSize);
    
    CAmount fee = totalFee / tx.vout.size();
    CAmount feeReminder = totalFee % tx.vout.size();
    
    for(auto& txOut : tx.vout)
    {
        txOut.nValue-=fee;
    }
    
    tx.vout.back().nValue-=feeReminder;
    
    return totalFee;
}
