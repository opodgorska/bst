// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/client.h>
#include <validation.h>
#include <policy/policy.h>
#include <utilstrencodings.h>
#include <stdint.h>
#include <amount.h>
#include <chainparams.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <univalue.h>
#include <boost/algorithm/string.hpp>

#include <data/datautils.h>
#include <data/processunspent.h>

#include <lottery/lotterytxs.h>

static std::string changeAddress("");

static UniValue callRPC(std::string args)
{
    std::vector<std::string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    std::string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    JSONRPCRequest request;
    request.strMethod = strMethod;
    request.params = RPCConvertValues(strMethod, vArgs);
    request.fHelp = false;

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        UniValue result = (*method)(request);
        return result;
    }
    catch (const UniValue& objError) {
        throw std::runtime_error(find_value(objError, "message").get_str());
    }
}

UniValue makebet(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VNUM, UniValue::VNUM});
	
	if (request.fHelp || request.params.size() != 2)
	throw std::runtime_error(
        "makebet \n"
        "\nCreates a bet transaction.\n"
        "Before this command walletpassphrase is required. \n"

        "\nArguments:\n"
        "1. \"number\"                      (numeric, required) A number to be drown in range from 0 to 1023 \n"
        "2. \"amount\"                      (numeric, required) Amount of money to be multiplied if you win or lose in other case. Max value of amount is half of block mining reward\n"

        "\nResult:\n"
        "\"txid\"                           (string) A hex-encoded transaction id\n"


        "\nExamples:\n"
        + HelpExampleCli("makebet", "33 0.05")
        + HelpExampleRpc("makebet", "33 0.05")
	);

    std::shared_ptr<CWallet> wallet = GetWallets()[0];
    if(wallet==nullptr)
    {
        throw std::runtime_error(std::string("No wallet found"));
    }
    CWallet* const pwallet=wallet.get();

    int betNumber = request.params[0].get_int();
    if(betNumber < 0 || betNumber >= MAX_BET_REWARD)
    {
        throw std::runtime_error(std::string("Bet number is out of range <0, ")+std::to_string(MAX_BET_REWARD)+std::string(">"));
    }
    const Consensus::Params& params = Params().GetConsensus();
    double blockSubsidy = static_cast<double>(GetBlockSubsidy(chainActive.Height(), params)/COIN);
    double betAmount = request.params[1].get_real();
    if(betAmount <= 0 || betAmount >= (ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy))
    {
        throw std::runtime_error(std::string("Amount is out of range <0, ")+std::to_string(ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy)+std::string(">"));
    }
    int mask = getMask(betNumber);
    constexpr size_t txSize=265;
    double fee;

    CCoinControl coin_control;
    coin_control.m_feerate.reset();
    coin_control.m_confirm_target = 2;
    coin_control.m_signal_bip125_rbf = false;
    FeeCalculation fee_calc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(*pwallet, 1000, coin_control, ::mempool, ::feeEstimator, &fee_calc));

    std::vector<std::string> addresses;
    ProcessUnspent processUnspent(pwallet, addresses);

    UniValue inputs(UniValue::VARR);
    if(!processUnspent.getUtxForAmount(inputs, feeRate, txSize, betAmount, fee))
    {
        throw std::runtime_error(std::string("Insufficient funds"));
    }

    if(changeAddress.empty())
    {
        changeAddress=getChangeAddress(pwallet);
    }

    UniValue sendTo(UniValue::VARR);
    
    UniValue bet(UniValue::VOBJ);
    bet.pushKV("betNumber", betNumber);
    bet.pushKV("betAmount", betAmount);
    bet.pushKV("mask", mask);
    sendTo.push_back(bet);

    int reward=maskToReward(mask);
    if(reward>MAX_BET_REWARD)
    {
        throw std::runtime_error(strprintf("Potential reward is greater than %d", MAX_BET_REWARD));
    }
    std::string msg(byte2str(reinterpret_cast<unsigned char*>(&reward),sizeof(reward)));
    
    UniValue betReward(UniValue::VOBJ);
    betReward.pushKV("data", msg);
    sendTo.push_back(betReward);

    UniValue change(UniValue::VOBJ);
    change.pushKV(changeAddress, computeChange(inputs, betAmount+fee));
    sendTo.push_back(change);

    MakeBetTxs tx(pwallet, inputs, sendTo);
    EnsureWalletIsUnlocked(pwallet);
    tx.signTx();
    std::string txid=tx.sendTx().get_str();

    return UniValue(UniValue::VSTR, txid);
}

UniValue getbet(const JSONRPCRequest& request)
{
	RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR});
	
	if (request.fHelp || request.params.size() != 2)
	throw std::runtime_error(
        "getbet \n"
        "\nTry to redeem a reward from the transaction created by makebet.\n"
        "Before this command walletpassphrase is required. \n"

        "\nArguments:\n"
        "1. \"txid\"         (string, required) The transaction id returned by makebet\n"
        "2. \"address\"      (string, required) The address to sent the reward\n"

        "\nResult:\n"
        "\"txid\"            (string) A hex-encoded transaction id\n"


        "\nExamples:\n"
        + HelpExampleCli("getbet", "\"123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB\"")
        + HelpExampleRpc("getbet", "\"123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB\"")
	);

    std::shared_ptr<CWallet> wallet = GetWallets()[0];
    if(wallet==nullptr)
    {
        throw std::runtime_error(std::string("No wallet found"));
    }
    CWallet* const pwallet=wallet.get();
    
    std::string txidIn = request.params[0].get_str();
    
    UniValue txPrev(UniValue::VOBJ);
    txPrev=GetBetTxs::findTx(txidIn);
    UniValue prevTxBlockHash(UniValue::VSTR);
    prevTxBlockHash=txPrev["blockhash"].get_str();
    
    constexpr int voutIdx=0;
    UniValue vout(UniValue::VOBJ);
    vout=txPrev["vout"][voutIdx];

    constexpr size_t txSize=265;
    CCoinControl coin_control;
    coin_control.m_feerate.reset();
    coin_control.m_confirm_target = 2;
    coin_control.m_signal_bip125_rbf = false;
    FeeCalculation fee_calc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(*pwallet, 1000, coin_control, ::mempool, ::feeEstimator, &fee_calc));
    double fee=static_cast<double>(feeRate.GetFee(txSize))/COIN;

    UniValue scriptPubKeyStr(UniValue::VSTR);
    scriptPubKeyStr=vout["scriptPubKey"]["hex"];
    int reward=getReward<int>(pwallet, scriptPubKeyStr.get_str());
    std::string amount=double2str(reward*vout["value"].get_real()-fee);
    
    UniValue txIn(UniValue::VOBJ);
    txIn.pushKV("txid", txidIn);
    txIn.pushKV("vout", voutIdx);

    std::string address = request.params[1].get_str();
    UniValue sendTo(UniValue::VOBJ);
    sendTo.pushKV("address", address);
    sendTo.pushKV("amount", amount);

    GetBetTxs tx(pwallet, txIn, sendTo, prevTxBlockHash);
    EnsureWalletIsUnlocked(pwallet);
    tx.signTx();
    std::string txid=tx.sendTx().get_str();

    return UniValue(UniValue::VSTR, txid);
}



static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "blockchain",         "makebet",                	        &makebet,             	        {"number", "amount"} },
    { "blockchain",         "getbet",                	        &getbet,             	        {"txid", "address"} },
};

void RegisterLotteryRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
