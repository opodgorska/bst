// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <key_io.h>
#include <lottery/lotterytxs.h>
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

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, UniValue& entry);

//creating a lottery transaction

class CScriptBetVisitor : public boost::static_visitor<bool>
{
private:
    int mask;
    int betNumber;
    CScript *script;
public:
    explicit CScriptBetVisitor(CScript *scriptin, char mask_, char betNumber_) : mask(mask_), betNumber(betNumber_), script(scriptin) {}

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const CKeyID &keyID) const {
        script->clear();
        
        std::vector<unsigned char> maskArray;
        std::vector<unsigned char> betNumberArray;
        type2array(mask, maskArray);
        type2array(betNumber, betNumberArray);

        *script << maskArray << OP_AND << betNumberArray << OP_EQUALVERIFY << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
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

CScript GetScriptForBetDest(const CTxDestination& dest, int mask, int betNumber)
{
    CScript script;

    boost::apply_visitor(CScriptBetVisitor(&script, mask, betNumber), dest);
    return script;
}

UniValue MakeBetTxs::getnewaddress(CTxDestination& dest, OutputType output_type)
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


MakeBetTxs::MakeBetTxs(CWallet* const pwallet_, const UniValue& inputs, const UniValue& sendTo, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_, int32_t txVersion_) 
                      : Txs(txVersion_, pwallet_, nLockTime_, rbfOptIn_, allowhighfees_)
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
        const UniValue& sendToObj = sendTo[idx].get_obj();

        std::vector<std::string> addrList = sendToObj.getKeys();

        if (addrList[0] == "data") 
        {
            const std::string addr = addrList[0];
            std::vector<unsigned char> data = ParseHexV(sendToObj[addr].getValStr(),"Data");

            CTxOut out(0, CScript() << OP_RETURN << data);
            rawTx.vout.push_back(out);
        }
        else if(addrList[0]=="betNumber")
        {
            std::string key = addrList[0];
            int betNumber = sendToObj[key].get_int();

            key = addrList[1];
            CAmount nAmount = AmountFromValue(sendToObj[key].getValStr());

            key = addrList[2];
            int mask = sendToObj[key].get_int();

            //we generate a new address type of OUTPUT_TYPE_LEGACY
            CTxDestination dest;
            getnewaddress(dest);

            redeemScript = GetScriptForBetDest(dest, mask, betNumber);
            pwallet->AddCScript(redeemScript);

            CScript scriptPubKey;
            scriptPubKey.clear();
            scriptPubKey << OP_HASH160 << ToByteVector(CScriptID(redeemScript)) << OP_EQUAL;

            CTxOut out(nAmount, scriptPubKey);
            rawTx.vout.push_back(out);
        }
        else 
        {
            const std::string addr = addrList[0];
            CTxDestination destination = DecodeDestination(addr);
            if (!IsValidDestination(destination)) 
            {
                throw std::runtime_error(std::string("Invalid Bitcoin address: ") + addr);
            }

            CScript scriptPubKey = GetScriptForDestination(destination);
            CAmount nAmount = AmountFromValue(sendToObj[addr].getValStr());

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

void MakeBetTxs::getOpReturnAccReward(double& rewardAcc, const CTransaction& tx, const UniValue& amount)
{
    int reward=0;
    char* rewardPtr=reinterpret_cast<char*>(&reward);
    for(size_t i=2;i<tx.vout[1].scriptPubKey.size();++i)
    {
        *rewardPtr=tx.vout[1].scriptPubKey[i];
        ++rewardPtr;
    }
    //std::cout<<"reward: "<<reward<<std::endl;
    rewardAcc+=(reward*amount.get_real());
}

//#define USE_POTENTIAL_REWARD 1
bool MakeBetTxs::checkBetRewardSum(double& rewardAcc, const CTransaction& tx, const Consensus::Params& params)
{
    double blockSubsidy = static_cast<double>(GetBlockSubsidy(chainActive.Height(), params)/COIN);
    int32_t txMakeBetVersion=(tx.nVersion ^ MAKE_BET_INDICATOR);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        UniValue amount(UniValue::VNUM);
        amount=ValueFromAmount(tx.vout[0].nValue);
        std::cout<<"vin.betAmount: "<<amount.get_real()<<std::endl;

//to control the potential reward use below commented code
#if defined(USE_POTENTIAL_REWARD)
        getOpReturnAccReward(rewardAcc, tx);
#else
        rewardAcc+=amount.get_real();
#endif
        std::cout<<"rewardAcc: "<<rewardAcc<<std::endl;
        if(rewardAcc>ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy)
        {
            std::cout<<"rewardAcc reached the limit\n";
            return false;
        }
    }
    return true;
}

/***********************************************************************/
//redeeming a lottery transaction

GetBetTxs::GetBetTxs(CWallet* const pwallet_, const UniValue& inputs, const UniValue& sendTo, const UniValue& prevTxBlockHash_, int64_t nLockTime_, bool rbfOptIn_, bool allowhighfees_) 
                    : Txs(pwallet_, nLockTime_, rbfOptIn_, allowhighfees_), prevTxBlockHash(prevTxBlockHash_)
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

    // scriptSig:    <sig> <sig...> <serialized_script>
    // scriptPubKey: HASH160 <hash> EQUAL

bool GetBetTxs::Sign1(const SigningProvider& provider, const CKeyID& address, const BaseSignatureCreator& creator, const CScript& scriptCode, std::vector<valtype>& ret, SigVersion sigversion)
{
    std::vector<unsigned char> vchSig;
    if (!creator.CreateSig(provider, vchSig, address, scriptCode, sigversion))
        return false;
    ret.push_back(vchSig);
    return true;
}

CScript GetBetTxs::PushAll(const std::vector<valtype>& values)
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

bool GetBetTxs::ProduceSignature(const BaseSignatureCreator& creator, const CScript& scriptPubKey, SignatureData& sigdata)
{
    std::vector<valtype> result;
    CScript redeemScript;
    std::vector<unsigned char> scriptPubKeyHashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
    pwallet->GetCScript(uint160(scriptPubKeyHashBytes), redeemScript);//get redeemScript by its hash

    sigdata.scriptWitness.stack.clear();
    
    std::vector<unsigned char> redeemScriptHashBytes(redeemScript.end()-22, redeemScript.end()-2);
    CKeyID keyID = CKeyID(uint160(redeemScriptHashBytes));//get hash of a pubKeyHash from redeemScript

    if (!Sign1(*pwallet, keyID, creator, redeemScript, result, SigVersion::BASE))
    {
        return false;
    }
    else
    {
        //result[0]<-sig
        
        CPubKey vch;
        pwallet->GetPubKey(keyID, vch);
        result.push_back(ToByteVector(vch));
        //result[1]<-pubKey
        
        std::string prevTxBlockHashStr=prevTxBlockHash.get_str();
        std::vector<unsigned char> binaryBlockHash(prevTxBlockHashStr.length()/2, 0);
        hex2bin(binaryBlockHash, prevTxBlockHashStr);
        std::cout<<"prevTxBlockHashStr: "<<prevTxBlockHashStr<<std::endl;
        std::vector<unsigned char> blockhash(binaryBlockHash.end()-4, binaryBlockHash.end());
        std::reverse(blockhash.begin(), blockhash.end());//revert here to match endianess in script
        result.push_back(blockhash);
        //result[2]<-blockhash

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

UniValue GetBetTxs::findTx(const std::string& txid)
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

bool GetBetTxs::txVerify(const CTransaction& tx, CAmount in, CAmount out)
{
    UniValue txPrev(UniValue::VOBJ);
    try
    {
        txPrev=GetBetTxs::findTx(tx.vin[0].prevout.hash.GetHex());
    }
    catch(...)
    {
        std::cout<<"GetBetTxs::txVerify findTx() failed\n";
        return false;
    }
    std::string blockhash=txPrev["blockhash"].get_str();

    std::string op_return_data=txPrev["vout"][1]["scriptPubKey"]["hex"].get_str().substr(4, 8);
    reverseEndianess(op_return_data);
    
    int opReturnReward = std::stoi(op_return_data,nullptr,16);
    std::cout<< "opReturnReward: " << opReturnReward <<std::endl;

    std::vector<unsigned char> blockhashInScript(tx.vin[0].scriptSig.end()-42, tx.vin[0].scriptSig.end()-38);
    std::reverse(blockhashInScript.begin(), blockhashInScript.end());
    std::string blockhashInScriptStr=HexStr(blockhashInScript.begin(), blockhashInScript.end());
    
    std::cout<<"blockhashInScriptStr: "<<blockhashInScriptStr<<std::endl;
    
    std::vector<unsigned char> mask_(tx.vin[0].scriptSig.end()-36, tx.vin[0].scriptSig.end()-32);
    const unsigned char op_and=*(tx.vin[0].scriptSig.end()-32);
    std::vector<unsigned char> betNumber_(tx.vin[0].scriptSig.end()-30, tx.vin[0].scriptSig.end()-26);
    const unsigned char op_equalverify_1=*(tx.vin[0].scriptSig.end()-26);
    const unsigned char op_dup=*(tx.vin[0].scriptSig.end()-25);
    const unsigned char op_hash160=*(tx.vin[0].scriptSig.end()-24);
    const unsigned char op_equalverify_2=*(tx.vin[0].scriptSig.end()-2);
    const unsigned char op_checksig=*(tx.vin[0].scriptSig.end()-1);
    
    int mask=0;
    array2type(mask_, mask);
    
    if(out > maskToReward(mask)*in)
    {
        std::cout<<"out > maskToReward(mask)*in\n";
        return false;
    }
    
    if(opReturnReward != maskToReward(mask))
    {
        std::cout<<"opReturnReward != maskToReward(mask)\n";
        return false;
    }
    
    if(opReturnReward>MAX_BET_REWARD || maskToReward(mask)>MAX_BET_REWARD)
    {
        std::cout<<"MAX_REWARD exceeded\n";
        return false;        
    }

    int betNumber=0;
    array2type(betNumber_, betNumber);

    std::vector<unsigned char> blockhashNumber_(tx.vin[0].scriptSig.end()-42, tx.vin[0].scriptSig.end()-38);
    int blockhashNumber=0;
    array2type(blockhashNumber_, blockhashNumber);
    
    printf("blockhashNumber: %x\n", blockhashNumber);
    printf("mask: %x\n", mask);
    printf("betNumber: %x\n", betNumber);
    bool a=((mask & blockhashNumber)==betNumber);
    std::cout<<"(mask & blockhashNumber)==betNumber: "<<a<<std::endl;
    printf("op_and: %x\n", op_and);
    std::cout<<"blockhash.substr(56, 8): "<<blockhash.substr(56, 8)<<std::endl;

    if((mask & blockhashNumber)==betNumber &&
       op_and==OP_AND &&
       op_equalverify_1==OP_EQUALVERIFY &&
       op_dup==OP_DUP &&
       op_hash160==OP_HASH160 &&
       op_equalverify_2==OP_EQUALVERIFY &&
       op_checksig==OP_CHECKSIG &&
       blockhashInScriptStr==blockhash.substr(56, 8))
    {
        return true;
    }

    return false;
}
