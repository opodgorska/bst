// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iomanip>
#include <numeric>
#include <rpc/server.h>
#include <rpc/client.h>
#include <validation.h>
#include <policy/policy.h>
#include <utilstrencodings.h>
#include <stdint.h>
#include <amount.h>
#include <chainparams.h>
#include <rpc/mining.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <univalue.h>
#include <boost/algorithm/string.hpp>

#include <data/datautils.h>
#include <data/processunspent.h>

#include <games/gamestxs.h>
#include <games/modulo/modulotxs.h>
#include <games/gamesutils.h>

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

static int straight;
static int split[33][2]=
{
    {1, 4},
    {4, 7},
    {7, 10},
    {10, 13},
    {13, 16},
    {16, 19},
    {19, 22},
    {22, 25},
    {25, 28},
    {28, 31},
    {31, 34},
    {2, 5},
    {5, 8},
    {8, 11},
    {11, 14},
    {14, 17},
    {17, 20},
    {20, 23},
    {23, 26},
    {26, 29},
    {29, 32},
    {32, 35},
    {3, 6},
    {6, 9},
    {9, 12},
    {12, 15},
    {15, 18},
    {18, 21},
    {21, 24},
    {24, 27},
    {27, 30},
    {30, 33},
    {33, 36}
};

static int street[12][3]=
{
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
    {10, 11, 12},
    {13, 14, 15},
    {16, 17, 18},
    {19, 20, 21},
    {22, 23, 24},
    {25, 26, 27},
    {28, 29, 30},
    {31, 32, 33},
    {34, 35, 36}
};

static int corner[22][4]=
{
    {1, 2, 4, 5},
    {4, 5, 7, 8},
    {7, 8, 10, 11},
    {10, 11, 13, 14},
    {13, 14, 16, 17},
    {16, 17, 19, 20},
    {19, 20, 22, 23},
    {22, 23, 25, 26},
    {25, 26, 28, 29},
    {28, 29, 31, 32},
    {31, 32, 34, 35},
    {2, 3, 5, 6},
    {5, 6, 8, 9},
    {8, 9, 11, 12},
    {11, 12, 14, 15},
    {14, 15, 17, 18},
    {17, 18, 20, 21},
    {20, 21, 23, 24},
    {23, 24, 26, 27},
    {26, 27, 29, 30},
    {29, 30, 32, 33},
    {32, 33, 35, 36}
};

static int line[11][6]=
{
    {1, 2, 3, 4, 5, 6},
    {4, 5, 6, 7, 8, 9},
    {7, 8, 9, 10, 11, 12},
    {10, 11, 12, 13, 14, 15},
    {13, 14, 15, 16, 17, 18},
    {16, 17, 18, 19, 20, 21},
    {19, 20, 21, 22, 23, 24},
    {22, 23, 24, 25, 26, 27},
    {25, 26, 27, 28, 29, 30},
    {28, 29, 30, 31, 32, 33},
    {31, 32, 33, 34, 35, 36}
};

static int column[3][12]=
{
    {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34},
    {2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35},
    {3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36}
};

static int dozen[3][12]=
{
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
    {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}
};

static int low[18]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 18};
static int high[18]={19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

static int even[18]={2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36};
static int odd[18]={1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35};

static int red[18]={2, 4, 6, 8, 10, 11, 13, 15, 17, 20, 22, 24, 26, 28, 29, 31, 33, 35};
static int black[18]={1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36};

static int getRouletteBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range)
{
    std::setlocale(LC_NUMERIC, "C");
    std::setprecision(9);
    reward = 0;
    size_t at=betTypePattern.find("@");
    if(at==std::string::npos)
    {
        throw std::runtime_error(std::string("Incomplete bet type"));
    }
    size_t stringLen = betTypePattern.find("+", at);
    amount=std::stod(betTypePattern.substr(at+1, stringLen-at-1));
    betType=betTypePattern.substr(0, at);
    if(betTypePattern.find("straight_")==0 || betTypePattern.find("STRAIGHT_")==0)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("straight_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for straight type are 1, 2, ...")+std::to_string(range));
        }
        if(betNum<1 || betNum>range)
        {
            throw std::runtime_error(std::string("Possible numbers for straight type are 1, 2, ...")+std::to_string(range));
        }
        straight = betNum;
        bet = &straight;
        len = 1;
        reward = range/len;
    }
    else if((betTypePattern.find("split_")==0 || betTypePattern.find("SPLIT_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("split_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for split type are 1, 2, ...33"));
        }
        if(betNum<1 || betNum>33)
        {
            throw std::runtime_error(std::string("Possible numbers for split type are 1, 2, ...33"));
        }
        bet = split[betNum-1];
        len = 2;
        reward = range/len;
    }
    else if((betTypePattern.find("street_")==0 || betTypePattern.find("STREET_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("street_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for street type are 1, 2, ...12"));
        }        
        if(betNum<1 || betNum>12)
        {
            throw std::runtime_error(std::string("Possible numbers for street type are 1, 2, ...12"));
        }
        bet = street[betNum-1];
        len = 3;
        reward = range/len;
    }
    else if((betTypePattern.find("corner_")==0 || betTypePattern.find("CORNER_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("corner_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for corner type are 1, 2, ...22"));
        }
        if(betNum<1 || betNum>22)
        {
            throw std::runtime_error(std::string("Possible numbers for corner type are 1, 2, ...22"));
        }
        bet = corner[betNum];
        len = 4;
        reward = range/len;
    }
    else if((betTypePattern.find("line_")==0 || betTypePattern.find("LINE_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("line_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for line type are 1, 2, ...11"));
        }
        if(betNum<1 || betNum>11)
        {
            throw std::runtime_error(std::string("Possible numbers for line type are 1, 2, ...11"));
        }
        bet = line[betNum-1];
        len = 6;
        reward = range/len;
    }
    else if((betTypePattern.find("column_")==0 || betTypePattern.find("COLUMN_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("column_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers for column type are 1, 2, 3"));
        }
        if(betNum<1 || betNum>3)
        {
            throw std::runtime_error(std::string("Possible numbers for column type are 1, 2, 3"));
        }
        bet = column[betNum-1];
        len = 12;
        reward = range/len;
    }
    else if((betTypePattern.find("dozen_")==0 || betTypePattern.find("DOZEN_")==0) && range==36)
    {
        int betNum;
        try
        {
            betNum=std::stoi(betTypePattern.substr(std::string("dozen_").length(), at));
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible dozen for line type are 1, 2, 3"));
        }
        if(betNum<1 || betNum>3)
        {
            throw std::runtime_error(std::string("Possible dozen for line type are 1, 2, 3"));
        }
        bet = dozen[betNum-1];
        len = 12;
        reward = range/len;
    }
    else if((betTypePattern.find("low")==0 || betTypePattern.find("LOW")==0) && range==36)
    {
        bet = low;
        len = 18;
        reward = range/len;
    }
    else if((betTypePattern.find("high")==0 || betTypePattern.find("HIGH")==0) && range==36)
    {
        bet = high;
        len = 18;
        reward = range/len;
    }
    else if((betTypePattern.find("even")==0 || betTypePattern.find("EVEN")==0) && range==36)
    {
        bet = even;
        len = 18;
        reward = range/len;
    }
    else if((betTypePattern.find("odd")==0 || betTypePattern.find("ODD")==0) && range==36)
    {
        bet = odd;
        len = 18;
        reward = range/len;
    }
    else if((betTypePattern.find("red")==0 || betTypePattern.find("RED")==0) && range==36)
    {
        bet = red;
        len = 18;
        reward = range/len;
    }
    else if((betTypePattern.find("black")==0 || betTypePattern.find("BLACK")==0) && range==36)
    {
        bet = black;
        len = 18;
        reward = range/len;
    }

    return stringLen;
}

static int getModuloBet(const std::string& betTypePattern, int*& bet, int& len, int& reward, double& amount, std::string& betType, int range)
{
    std::setlocale(LC_NUMERIC, "C");
    std::setprecision(9);
    reward = 0;
    size_t at=betTypePattern.find("@");
    if(at==std::string::npos)
    {
        throw std::runtime_error(std::string("Incomplete bet type"));
    }
    size_t stringLen = betTypePattern.find("+", at);
    amount=std::stod(betTypePattern.substr(at+1, stringLen-at-1));
    betType=betTypePattern.substr(0, at);

    if( (betType.find_first_not_of( "0123456789" ) == std::string::npos) )
    {
        int betNum;
        try
        {
            betNum=std::stoi(betType);
        }
        catch(...)
        {
            throw std::runtime_error(std::string("Possible numbers are 1, 2, ...")+std::to_string(range));
        }
        if(betNum<1 || betNum>range)
        {
            throw std::runtime_error(std::string("Possible numbers are 1, 2, ...")+std::to_string(range));
        }
        straight = betNum;
        bet = &straight;
        len = 1;
        reward = range/len;
    }
    
    return stringLen;
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

    int range=36;//by default roulette range
    if(!request.params[1].isNull())
    {
        range=request.params[1].get_int();
    }

    for(int i=0;true;++i)
    {
        int reward;
        double betAmount;
        int* betArray;
        int betLen;
        std::string betType;
        size_t len;

        if(range == 36)
        {
            len=getRouletteBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
        }
        else
        {
            len=getModuloBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
        }
        
        if(reward <= 0 || reward > MAX_REWARD)
        {
            throw std::runtime_error(std::string("Incorrect type of bet"));
        }

        if(betAmount <= 0 || (static_cast<CAmount>(betAmount*COIN)*static_cast<CAmount>(reward) > MAX_PAYOFF))
        {
            throw std::runtime_error(std::string("Amount is out of range"));
        }

        std::vector<int> betVec;
        betArrays.push_back(betVec);
        std::copy(&betArray[0], &betArray[betLen], back_inserter(betArrays[i]));
        betAmounts.push_back(betAmount);
        betTypes.push_back(betType);
        
        if(len==std::string::npos)
        {
            break;
        }
        betTypePattern=betTypePattern.substr(len+1);
    }

    size_t opReturnSize=0;
    for(std::vector<std::string>::iterator it=betTypes.begin();it!=betTypes.end();++it)
    {
        opReturnSize+=it->size();
    }
    size_t txSize=265+
    (36*(betTypes.size()-1))+//script size
    opReturnSize;
    
    double fee;

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

    FeeCalculation fee_calc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(*pwallet, 1000, coin_control, ::mempool, ::feeEstimator, &fee_calc));

    std::vector<std::string> addresses;
    ProcessUnspent processUnspent(pwallet, addresses);

    UniValue inputs(UniValue::VARR);
    double betAmountAcc=std::accumulate(betAmounts.begin(), betAmounts.end(), 0.0);
    if(!processUnspent.getUtxForAmount(inputs, feeRate, txSize, betAmountAcc, fee))
    {
        throw std::runtime_error(std::string("Insufficient funds"));
    }

    if(fee>(static_cast<double>(maxTxFee)/COIN))
    {
        fee=(static_cast<double>(maxTxFee)/COIN);
    }

    if(changeAddress.empty())
    {
        changeAddress=getChangeAddress(pwallet);
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
    UniValue change(UniValue::VOBJ);
    change.pushKV(changeAddress, computeChange(inputs, betAmountAcc+fee));
    sendTo.push_back(change);

    MakeBetTxs tx(pwallet, inputs, sendTo, (MAKE_MODULO_GAME_INDICATOR | CTransaction::CURRENT_VERSION), 0, coin_control.m_signal_bip125_rbf.get_value_or(false), false);
    EnsureWalletIsUnlocked(pwallet);
    tx.signTx();
    std::string txid=tx.sendTx().get_str();

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
        txIn.pushKV("vout", voutIdx);

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
    { "blockchain",         "makebet",                      &makebet,                   {"type_of_bet", "range", "replaceable", "conf_target", "estimate_mode"} },
    { "blockchain",         "getbet",                       &getbet,                    {"txid", "address", "replaceable", "conf_target", "estimate_mode"} },
};

void RegisterGameRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
