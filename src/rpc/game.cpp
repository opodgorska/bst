// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <numeric>
#include <rpc/server.h>
#include <rpc/client.h>
#include <consensus/validation.h>
#include <validation.h>
#include <policy/policy.h>
#include <utilstrencodings.h>
#include <stdint.h>
#include <amount.h>
#include <chainparams.h>
#include <net.h>
#include <rpc/mining.h>
#include <utilmoneystr.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <univalue.h>
#include <boost/algorithm/string.hpp>

#include <data/datautils.h>
#include <data/processunspent.h>

#include <games/gamestxs.h>
#include <games/modulo/modulotxs.h>
#include <games/gamesutils.h>
#include <games/modulo/moduloutils.h>

using namespace modulo;

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
	if (request.fHelp || request.params.size() < 1 || request.params.size() > 5)
	throw std::runtime_error(
        "makebet \n"
        "\nCreates a bet transaction.\n"
        "Before this command walletpassphrase is required. \n"

        "\nArguments:\n"
        "1. \"type_of_bet\"                 (string, required) Defines a combination of bet type and number along with the bet amount. Roulette specific bets are available for default range\n"
        "2. \"range\"                       (numeric, optional) Defines a range of numbers <1, range> to be drawn. Default range is set to 36 meaning a roulette\n"
        "3. replaceable                     (boolean, optional) Allow this transaction to be replaced by a transaction with higher fees via BIP 125\n"
        "4. conf_target                     (numeric, optional) Confirmation target (in blocks)\n"
        "5. \"estimate_mode\"               (string, optional, default=UNSET) The fee estimate mode, must be one of:\n"
        "       \"UNSET\"\n"
        "       \"ECONOMICAL\"\n"
        "       \"CONSERVATIVE\"\n"

        "\nResult:\n"
        "\"txid\"                           (string) A hex-encoded transaction id\n"


        "\nExamples:\n"
        + HelpExampleCli("makebet", "straight_2@0.1+street_3@0.05")
        + HelpExampleRpc("makebet", "straight_2@0.1+street_3@0.05")
	);

    std::shared_ptr<CWallet> wallet = GetWallets()[0];
    if(wallet==nullptr)
    {
        throw std::runtime_error(std::string("No wallet found"));
    }
    CWallet* const pwallet=wallet.get();

    std::vector<double> betAmounts;
    std::vector<std::string> betTypes;
    std::vector<std::vector<int> > betArrays;

    std::string betTypePattern=request.params[0].get_str();

    bool isRoulette=true;
    int range=36;//by default roulette range
    if(!request.params[1].isNull())
    {
        range=request.params[1].get_int();
        isRoulette=false;
    }

    parseBetType(betTypePattern, range, betAmounts, betTypes, betArrays, isRoulette);

    CCoinControl coin_control;
    if (!request.params[2].isNull())
    {
        coin_control.m_signal_bip125_rbf = request.params[3].get_bool();
    }

    if (!request.params[3].isNull())
    {
        coin_control.m_confirm_target = ParseConfirmTarget(request.params[4]);
    }

    if (!request.params[4].isNull())
    {
        if (!FeeModeFromString(request.params[4].get_str(), coin_control.m_fee_mode)) {
            throw std::runtime_error("Invalid estimate_mode parameter");
        }
    }

    UniValue sendTo(UniValue::VARR);
    UniValue rangeObj(UniValue::VOBJ);
    rangeObj.pushKV("argument", range);
    sendTo.push_back(rangeObj);
    for(size_t i=0;i<betArrays.size();++i)
    {
        UniValue obj(UniValue::VOBJ);
        UniValue betNumbers(UniValue::VARR);
        for(size_t j=0;j<betArrays[i].size();++j)
        {
            betNumbers.push_back(betArrays[i][j]);
        }
        obj.pushKV("betNumbers", betNumbers);
        obj.pushKV("betAmount", double2str(betAmounts[i]));

        sendTo.push_back(obj);
    }
    std::string arg=int2hex(range)+std::string("_");
    std::string msg=HexStr(arg.begin(), arg.end());
    std::string plus_msg("+");
    for(size_t i=0;i<betArrays.size();++i)
    {
        msg+=HexStr(betTypes[i].begin(), betTypes[i].end());
        if(i<betArrays.size()-1)
        {
            msg+=HexStr(plus_msg.begin(), plus_msg.end());
        }
    }
    UniValue opReturn(UniValue::VOBJ);
    opReturn.pushKV("data", msg);
    sendTo.push_back(opReturn);
    
    std::vector<CRecipient> vecSend;
    createMakeBetDestination(pwallet, sendTo, vecSend);

    LOCK2(cs_main, pwallet->cs_wallet);

    CAmount curBalance = pwallet->GetBalance();

    CReserveKey reservekey(pwallet);
    CAmount nFeeRequired;
    int nChangePosInOut=betArrays.size()+1;
    std::string strFailReason;
    CTransactionRef tx;

    EnsureWalletIsUnlocked(pwallet);

    if(!pwallet->CreateTransaction(vecSend, nullptr, tx, reservekey, nFeeRequired, nChangePosInOut, strFailReason, coin_control, true, true))
    {
        if (nFeeRequired > curBalance)
        {
            strFailReason = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        }        
        throw std::runtime_error(std::string("CreateTransaction failed with reason: ")+strFailReason);
    }

    CValidationState state;
    if(!pwallet->CommitTransaction(tx, {}, {}, reservekey, g_connman.get(), state))
    {
        throw std::runtime_error(std::string("CommitTransaction failed with reason: ")+FormatStateMessage(state));
    }

    std::string txid=tx->GetHash().GetHex();
    return UniValue(UniValue::VSTR, txid);
}

UniValue getbet(const JSONRPCRequest& request)
{
	if (request.fHelp || request.params.size() < 2 || request.params.size() > 5)
	throw std::runtime_error(
        "getbet \n"
        "\nTry to redeem a reward from the transaction created by makebet.\n"
        "Before this command walletpassphrase is required. \n"

        "\nArguments:\n"
        "1. \"txid\"                        (string, required) The transaction id returned by makebet\n"
        "2. \"address\"                     (string, required) The address to sent the reward\n"
        "3. replaceable                     (boolean, optional) Allow this transaction to be replaced by a transaction with higher fees via BIP 125\n"
        "4. conf_target                     (numeric, optional) Confirmation target (in blocks)\n"
        "5. \"estimate_mode\"               (string, optional, default=UNSET) The fee estimate mode, must be one of:\n"
        "       \"UNSET\"\n"
        "       \"ECONOMICAL\"\n"
        "       \"CONSERVATIVE\"\n"

        "\nResult:\n"
        "\"txid\"            (string) A hex-encoded winning transaction ids and/or errors for lost bets\n"


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
    txPrev=findTx(txidIn);
    UniValue prevTxBlockHash(UniValue::VSTR);
    prevTxBlockHash=txPrev["blockhash"].get_str();

    std::string address = request.params[1].get_str();

    CCoinControl coin_control;
    if (!request.params[2].isNull())
    {
        coin_control.m_signal_bip125_rbf = request.params[2].get_bool();
    }

    if (!request.params[3].isNull())
    {
        coin_control.m_confirm_target = ParseConfirmTarget(request.params[3]);
    }

    if (!request.params[4].isNull())
    {
        if (!FeeModeFromString(request.params[4].get_str(), coin_control.m_fee_mode)) {
            throw std::runtime_error("Invalid estimate_mode parameter");
        }
    }

    FeeCalculation fee_calc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(*pwallet, 1000, coin_control, ::mempool, ::feeEstimator, &fee_calc));

    std::string txid;
    size_t nPrevOut=txPrev["vout"].size()-2;
    for(size_t voutIdx=0;voutIdx<nPrevOut;++voutIdx)
    {
        UniValue vout(UniValue::VOBJ);
        vout=txPrev["vout"][voutIdx];

        UniValue scriptPubKeyStr(UniValue::VSTR);
        scriptPubKeyStr=vout["scriptPubKey"]["hex"];

        const CScript redeemScript = getRedeemScript(pwallet, scriptPubKeyStr.get_str());
        size_t redeemScriptSize=getRedeemScriptSize(redeemScript);

        size_t txSize=200+redeemScriptSize;
        double fee=static_cast<double>(feeRate.GetFee(txSize))/COIN;
        if(fee>(static_cast<double>(maxTxFee)/COIN))
        {
            fee=(static_cast<double>(maxTxFee)/COIN);
        }

        int reward=getReward(pwallet, scriptPubKeyStr.get_str());
        std::string amount=double2str(reward*vout["value"].get_real()-fee);

        UniValue txIn(UniValue::VOBJ);
        txIn.pushKV("txid", txidIn);
        txIn.pushKV("vout", static_cast<int>(voutIdx));

        UniValue sendTo(UniValue::VOBJ);
        sendTo.pushKV("address", address);
        sendTo.pushKV("amount", amount);

        ModuloOperation operation;
        GetBetTxs tx(pwallet, txIn, sendTo, prevTxBlockHash, &operation, 0, coin_control.m_signal_bip125_rbf.get_value_or(false));
        EnsureWalletIsUnlocked(pwallet);
        try
        {
            tx.signTx();
        }
        catch(std::exception const& e)
        {
            if(!txid.empty())
            {
                txid+=std::string("\n");
            }
            txid+=e.what();
            continue;
        }
        catch(...)
        {
            if(!txid.empty())
            {
                txid+=std::string("\n");
            }
            txid+=std::string("unknown exception occured");
            continue;
        }
        
        if(!txid.empty())
        {
            txid+=std::string("\n");
        }
        txid+=tx.sendTx().get_str();
    }

    return UniValue(UniValue::VSTR, txid);
}



static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "wallet",             "makebet",                      &makebet,                   {"type_of_bet", "range", "replaceable", "conf_target", "estimate_mode"} },
    { "wallet",             "getbet",                       &getbet,                    {"txid", "address", "replaceable", "conf_target", "estimate_mode"} },
};

void RegisterGameRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
