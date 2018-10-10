// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <index/txindex.h>
#include <key_io.h>
#include <logging.h>
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
    uint256 hash;
    hash.SetHex(txid);
    CBlockIndex* blockindex = nullptr;
    CTransactionRef tx;
    uint256 hash_block;
    UniValue result(UniValue::VOBJ);

    if (g_txindex)
    {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (hash == Params().GenesisBlock().hashMerkleRoot)
    {
        // Special exception for the genesis block coinbase transaction
        throw std::runtime_error(std::string("The genesis block coinbase is not considered an ordinary transaction and cannot be retrieved"));
    }

    if (GetTransaction(hash, tx, Params().GetConsensus(), hash_block, true, blockindex)) 
    {
        TxToJSON(*tx, hash_block, result);
    }
    
    if(hash_block.IsNull())
    {
        throw std::runtime_error(std::string("Transaction not in blockchain"));
    }
    
    return result;
}
