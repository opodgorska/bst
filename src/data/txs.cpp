#include <consensus/validation.h>
#include <key_io.h>
#include <net.h>
#include <policy/rbf.h>
#include <rpc/server.h>
#include <validation.h>

#include <future>
#include <validation.h>
#include <data/txs.h>

Txs::Txs(CWallet* const pwallet_, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_) 
        : pwallet(pwallet_), nLockTime(nLockTime_), rbfOptIn(rbfOptIn_), allowhighfees(allowhighfees_) {}

Txs::Txs(int32_t txVersion_, CWallet* const pwallet_, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_)
        : mtx(txVersion_), pwallet(pwallet_), nLockTime(nLockTime_), rbfOptIn(rbfOptIn_), allowhighfees(allowhighfees_) {}

Txs::~Txs() {}

UniValue Txs::createTx(const UniValue& inputs, const UniValue& sendTo)
{
    return createTxImp(inputs, sendTo);
}

UniValue Txs::signTx()
{
    return signTxImp();
}

UniValue Txs::sendTx()
{
    return sendTxImp();
}

UniValue Txs::sendTxImp()
{
    std::promise<void> promise;

    CTransactionRef tx(MakeTransactionRef(std::move(mtx)));
    const uint256& hashTx = tx->GetHash();

    CAmount nMaxRawTxFee = maxTxFee;
    if(allowhighfees)
    {
        nMaxRawTxFee = 0;
    }

    { // cs_main scope
    LOCK(cs_main);
    CCoinsViewCache &view = *pcoinsTip;
    bool fHaveChain = false;
    for (size_t o = 0; !fHaveChain && o < tx->vout.size(); o++) 
    {
        const Coin& existingCoin = view.AccessCoin(COutPoint(hashTx, o));
        fHaveChain = !existingCoin.IsSpent();
    }
    bool fHaveMempool = mempool.exists(hashTx);
    if (!fHaveMempool && !fHaveChain) 
    {
        // push to local node and sync with wallets
        CValidationState state;
        bool fMissingInputs;
        if (!AcceptToMemoryPool(mempool, state, std::move(tx), &fMissingInputs,
                                nullptr, // plTxnReplaced
                                false, // bypass_limits
                                nMaxRawTxFee)) 
        {
            if (state.IsInvalid()) 
            {
                throw std::runtime_error(FormatStateMessage(state));
            } 
            else 
            {
                if (fMissingInputs) 
                {
                    throw std::runtime_error(std::string("Missing inputs"));
                }
                throw std::runtime_error(FormatStateMessage(state));
            }
        } 
        else 
        {
            // If wallet is enabled, ensure that the wallet has been made aware
            // of the new transaction prior to returning. This prevents a race
            // where a user might call sendrawtransaction with a transaction
            // to/from their wallet, immediately call some wallet RPC, and get
            // a stale result because callbacks have not yet been processed.
            CallFunctionInValidationInterfaceQueue([&promise] {
                promise.set_value();
            });
        }
    } 
    else if (fHaveChain) 
    {
        throw std::runtime_error(std::string("transaction already in block chain"));
    } 
    else 
    {
        // Make sure we don't block forever if re-sending
        // a transaction already in mempool.
        promise.set_value();
    }

    } // cs_main

    promise.get_future().wait();

    if(!g_connman)
        throw std::runtime_error(std::string("Error: Peer-to-peer functionality missing or disabled"));

    CInv inv(MSG_TX, hashTx);
    g_connman->ForEachNode([&inv](CNode* pnode)
    {
        pnode->PushInventory(inv);
    });

    return hashTx.GetHex();
}

UniValue Txs::getnewaddress(CTxDestination& dest, OutputType output_type)
{
    LOCK2(cs_main, pwallet->cs_wallet);

    if (!pwallet->IsLocked()) {
        pwallet->TopUpKeyPool();
    }

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwallet->GetKeyFromPool(newKey))
    {
        throw std::runtime_error(std::string("Error: Keypool ran out, please call keypoolrefill first"));
    }
    pwallet->LearnRelatedScripts(newKey, output_type);
    dest = GetDestinationForKey(newKey, output_type);

    std::string strAccount("");
    pwallet->SetAddressBook(dest, strAccount, "receive");

    return EncodeDestination(dest);
}

// scriptSig:    <sig> <sig...> <serialized_script>
// scriptPubKey: HASH160 <hash> EQUAL
bool Txs::Sign1(const SigningProvider& provider, const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
{
    std::vector<unsigned char> vchSig;
    if (!creator.CreateSig(provider, vchSig, address, scriptCode, sigversion))
        return false;
    ret.push_back(vchSig);
    return true;
}

CScript Txs::PushAll(const std::vector<valtype>& values)
{
    CScript result;
    for (const valtype& v : values) {
        if (v.size() == 0) {
            result << OP_0;
        } else if (v.size() == 1 && v[0] >= 1 && v[0] <= 16) {
            result << CScript::EncodeOP_N(v[0]);
        } else {
            result << v;
        }
    }
    return result;
}
