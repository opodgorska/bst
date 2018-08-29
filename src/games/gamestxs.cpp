// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <games/gamestxs.h>
#include <games/gamesutils.h>
#include <policy/rbf.h>
#include <rpc/rawtransaction.h>
#include <index/txindex.h>
#include <rpc/server.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <validation.h>

#include <policy/policy.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>

//creating game transaction

class CScriptBetVisitor : public boost::static_visitor<bool>
{
private:
    int argument;
    std::vector<int> betNumbers;
    CScript *script;
public:
    explicit CScriptBetVisitor(CScript *scriptin, const std::vector<int>& betNumbers_, int argument_) : argument(argument_), betNumbers(betNumbers_), script(scriptin) {}

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const CKeyID &keyID) const {
        script->clear();
        std::vector<unsigned char> argumentArray;
        std::vector<unsigned char> betNumberArray;
        size_t numbersCount=betNumbers.size();
        
        type2array(argument, argumentArray);
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG << OP_IF;
        for(size_t i=0;i<numbersCount-1;++i)
        {
            type2array(betNumbers[i], betNumberArray);
            *script << OP_DUP << betNumberArray << OP_EQUAL << OP_IF << OP_DROP << OP_TRUE << OP_ELSE;
        }

        type2array(betNumbers[numbersCount-1], betNumberArray);
        *script << betNumberArray << OP_EQUALVERIFY << OP_TRUE;

        for(size_t i=0;i<numbersCount-1;++i)
        {        
            *script << OP_ENDIF;
        }

        *script << OP_ELSE << OP_DROP << OP_FALSE << OP_ENDIF << argumentArray << OP_DROP;
        return true;
    }

    bool operator()(const CScriptID &scriptID) const {
        script->clear();
        return false;
    }

    bool operator()(const WitnessV0KeyHash& id) const {
        script->clear();
        return false;
    }

    bool operator()(const WitnessV0ScriptHash& id) const {
        script->clear();
        return false;
    }

    bool operator()(const WitnessUnknown& id) const {
        script->clear();
        return false;
    }
};

CScript GetScriptForBetDest(const CTxDestination& dest, const std::vector<int>& betNumbers, int argument)
{
    CScript script;

    boost::apply_visitor(CScriptBetVisitor(&script, betNumbers, argument), dest);
    return script;
}

MakeBetTxs::MakeBetTxs(CWallet* const pwallet_, const UniValue& inputs, const UniValue& sendTo, int32_t txVersion_, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_) 
                      : Txs(txVersion_, pwallet_, nLockTime_, rbfOptIn_, allowhighfees_), isNewAddrGenerated(false)
{
    createTxImp(inputs, sendTo);
}

MakeBetTxs::~MakeBetTxs() {}

UniValue MakeBetTxs::createTxImp(const UniValue& inputs, const UniValue& sendTo)
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

    for (unsigned int idx = 0; idx < sendTo.size(); idx++)
    {
        int argument;
        std::vector<int> betNumbers;
        const UniValue& sendToObj = sendTo[idx].get_obj();
        std::vector<std::string> addrList = sendToObj.getKeys();
        
        for (const std::string &name_ : addrList)
        {
            if (name_ == "data") 
            {
                std::vector<unsigned char> data = ParseHexV(sendToObj[name_].getValStr(),"Data");

                CTxOut out(0, CScript() << OP_RETURN << data);
                rawTx.vout.push_back(out);
            }
            else if(name_=="argument")
            {
                argument = sendToObj[name_].get_int();
            }
            else if(name_=="betNumbers")
            {
                for(size_t i=0;i<sendToObj[name_].size();++i)
                {
                    betNumbers.push_back(sendToObj[name_][i].get_int());
                }
            }
            else if(name_=="betAmount")
            {
                CAmount nAmount = AmountFromValue(sendToObj[name_].getValStr());

                //we generate a new address type of OUTPUT_TYPE_LEGACY
                if(!isNewAddrGenerated)
                {
                    getnewaddress(dest);
                    isNewAddrGenerated=true;
                }

                redeemScript = GetScriptForBetDest(dest, betNumbers, argument);
                if(!pwallet->AddCScript(redeemScript))
                {
                    throw std::runtime_error(std::string("Adding script failed"));
                }

                CScript scriptPubKey;
                scriptPubKey.clear();
                scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;

                CTxOut out(nAmount, scriptPubKey);
                rawTx.vout.push_back(out);
            }
            else 
            {
                CTxDestination destination = DecodeDestination(name_);
                if (!IsValidDestination(destination)) 
                {
                    throw std::runtime_error(std::string("Invalid Bitcoin address: ") + name_);
                }

                CScript scriptPubKey = GetScriptForDestination(destination);
                CAmount nAmount = AmountFromValue(sendToObj[name_].getValStr());

                CTxOut out(nAmount, scriptPubKey);
                rawTx.vout.push_back(out);
            }
        }
    }

    if (rbfOptIn != SignalsOptInRBF(rawTx)) 
    {
        throw std::runtime_error(std::string("Invalid parameter combination: Sequence number(s) contradict replaceable option"));
    }
    return EncodeHexTx(rawTx);
}

UniValue MakeBetTxs::signTxImp()
{
    // Sign the transaction
    UniValue prevTxsUnival;
    prevTxsUnival.setNull();
    const UniValue hashType(std::string("ALL"));

    LOCK2(cs_main, pwallet->cs_wallet);
    return SignTransaction(mtx, prevTxsUnival, pwallet, false, hashType);
}

UniValue MakeBetTxs::getTx()
{
    return EncodeHexTx(mtx);
}

/***********************************************************************/
//redeeming transaction

GetBetTxs::GetBetTxs(CWallet* const pwallet_, const UniValue& inputs, const UniValue& sendTo, const UniValue& prevTxBlockHash_, ArgumentOperation* operation_, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_) 
                    : Txs(pwallet_, nLockTime_, rbfOptIn_, allowhighfees_), prevTxBlockHash(prevTxBlockHash_), operation(operation_)
{
    createTxImp(inputs, sendTo);
}

GetBetTxs::~GetBetTxs() {}

UniValue GetBetTxs::createTxImp(const UniValue& input, const UniValue& sendTo)
{
    CMutableTransaction& rawTx=mtx;

    if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
    {
        throw std::runtime_error(std::string("Invalid parameter, locktime out of range"));
    }
    rawTx.nLockTime = nLockTime;
    
    //unsigned int idx = 0;
    //for (unsigned int idx = 0; idx < inputs.size(); idx++)
    {
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


    {
        const std::string& name_ = sendTo["address"].getValStr();
        CTxDestination destination = DecodeDestination(name_);

        if (!IsValidDestination(destination)) 
        {
            throw std::runtime_error(std::string("Invalid Bitcoin address: ") + name_);
        }

        CScript scriptPubKey = GetScriptForDestination(destination);
        const std::string& amount = sendTo["amount"].getValStr();
        CAmount nAmount = AmountFromValue(amount);

        CTxOut out(nAmount, scriptPubKey);
        rawTx.vout.push_back(out);
    }

    if (rbfOptIn != SignalsOptInRBF(rawTx)) 
    {
        throw std::runtime_error(std::string("Invalid parameter combination: Sequence number(s) contradict replaceable option"));
    }
    
    return EncodeHexTx(rawTx);
}

UniValue GetBetTxs::signTxImp()
{
    // Sign the transaction
    const UniValue hashType(std::string("ALL"));
    LOCK2(cs_main, pwallet->cs_wallet);
    return SignRedeemBetTransaction(hashType);
}

UniValue GetBetTxs::SignRedeemBetTransaction(const UniValue hashType)
{
    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mtx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    int nHashType = SIGHASH_ALL;
    if (!hashType.isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = hashType.get_str();
        if (mapSigHashValues.count(strHashType)) {
            nHashType = mapSigHashValues[strHashType];
        } 
        else
        {
            throw std::runtime_error(std::string("Invalid sighash param"));
        }
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);
    // Sign what we can:
    unsigned int i = 0;
    //for (unsigned int i = 0; i < mtx.vin.size(); i++)
    {
        CTxIn& txin = mtx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) 
        {
            throw std::runtime_error(std::string("Input not found or already spent"));
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata;
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.vout.size())) 
        {
            ProduceSignature(MutableTransactionSignatureCreator(&mtx, i, amount, nHashType), prevPubKey, sigdata);
        }

        UpdateInput(txin, sigdata);

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, amount), &serror)) 
        {
            if (serror == SCRIPT_ERR_INVALID_STACK_OPERATION) 
            {
                // Unable to sign input and verification failed (possible attempt to partially sign).
                throw std::runtime_error(std::string("Unable to sign input, invalid stack size (possibly missing key)"));
            } else 
            {
                throw std::runtime_error(ScriptErrorString(serror));
            }
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(mtx));
    result.pushKV("complete", fComplete);
    if (!vErrors.empty()) 
    {
        result.pushKV("errors", vErrors);
    }

    return result;
}

unsigned int GetBetTxs::getMakeTxBlockHash(unsigned int argument)
{
    std::string makeTxBlockHashStr=prevTxBlockHash.get_str();
    std::vector<unsigned char> binaryBlockHash(makeTxBlockHashStr.length()/2, 0);
    hex2bin(binaryBlockHash, makeTxBlockHashStr);
    std::vector<unsigned char> blockhash(binaryBlockHash.end()-4, binaryBlockHash.end());
    
    unsigned int blockhashTmp;
    array2typeRev(blockhash, blockhashTmp);

    operation->setArgument(argument);
    return (*operation)(blockhashTmp);
}

bool GetBetTxs::ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata)
{
    std::vector<valtype> result;
    CScript redeemScript;
    std::vector<unsigned char> scriptPubKeyHashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
    pwallet->GetCScript(uint160(scriptPubKeyHashBytes), redeemScript);//get redeemScript by its hash

    sigdata.scriptWitness.stack.clear();

    std::vector<unsigned char> redeemScriptHashBytes(redeemScript.begin()+3, redeemScript.begin()+23);
    CKeyID keyID = CKeyID(uint160(redeemScriptHashBytes));//get hash of a pubKeyHash from redeemScript

    unsigned int argument = getArgument(redeemScript);
    unsigned int blockhashTmp=getMakeTxBlockHash(argument);
    std::vector<unsigned char> blockhash;
    type2array(blockhashTmp, blockhash);
    
    result.push_back(blockhash);
    //result[0]<-blockhash

    if (!Sign1(*pwallet, keyID, creator, redeemScript, result, SigVersion::BASE))
    {
        return false;
    }
    else
    {
        //result[1]<-sig
        
        CPubKey vch;
        pwallet->GetPubKey(keyID, vch);
        result.push_back(ToByteVector(vch));
        //result[2]<-pubKey

        result.push_back(ToByteVector(redeemScript));
        //result[3]<-redeemScript
    }

    sigdata.scriptSig = PushAll(result);

    // Test solution
    return VerifyScript(sigdata.scriptSig, scriptPubKey, &sigdata.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, creator.Checker());
}

UniValue GetBetTxs::getTx()
{
    return EncodeHexTx(mtx);
}

size_t getRedeemScriptSize(const CScript& redeemScript)
{
    CScript::const_iterator it_end=redeemScript.end();
    CScript::const_iterator it_begin=redeemScript.begin();
    return it_end-it_begin+1;
}

CScript getRedeemScript(CWallet* const pwallet, const std::string& scriptPubKeyStr)
{
    std::vector<unsigned char> scriptPubKeyChr(scriptPubKeyStr.length()/2, 0);
    hex2bin(scriptPubKeyChr, scriptPubKeyStr);
    CScript scriptPubKey(scriptPubKeyChr.begin(), scriptPubKeyChr.end());
    CScript redeemScript;
    std::vector<unsigned char> scriptPubKeyHashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
    pwallet->GetCScript(uint160(scriptPubKeyHashBytes), redeemScript);//get redeemScript by its hash
    
    return redeemScript;
}

int getReward(CWallet* const pwallet, const std::string& scriptPubKeyStr)
{        
    CScript redeemScript = getRedeemScript(pwallet, scriptPubKeyStr);
    unsigned int argument = getArgument(redeemScript);

    int numOfBetsNumbers=0;
    CScript::const_iterator it_end=redeemScript.end()-1;
    
    it_end-=6;
    
    if(*it_end!=OP_ENDIF)
    {
        return 0;
    }
    --it_end;
    if(*it_end!=OP_FALSE)
    {
        return 0;
    }
    --it_end;
    if(*it_end!=OP_DROP)
    {
        return 0;
    }
    --it_end;
    if(*it_end!=OP_ELSE)
    {
        return 0;
    }
    --it_end;
    for(CScript::const_iterator it=it_end;it>it_end-18;--it)
    {
        //printf("const_iterator: %x\n", *it);
        if(OP_TRUE==*it)
        {
            numOfBetsNumbers=it_end-it+1;
            it_end=it;
            break;
        }
        else if(OP_ENDIF!=*it)
        {
            return 0;
        }
    }
    --it_end;
    if(*it_end!=OP_EQUALVERIFY)
    {
        return 0;
    }
    
    return argument/numOfBetsNumbers;
}

unsigned int getArgument(const CScript& redeemScript)
{
    CScript::const_iterator it_end=redeemScript.end()-1;
    
    std::vector<unsigned char> argument_(it_end-4, it_end);
    
    unsigned int argument;
    array2type(argument_, argument);
    
    return argument;
}
