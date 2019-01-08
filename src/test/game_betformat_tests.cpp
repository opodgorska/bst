#include <test/test_bitcoin.h>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>

#include "chainparams.h"
#include "data/datautils.h"
#include "games/gamestxs.h"
#include "games/modulo/modulotxs.h"
#include "games/modulo/moduloutils.h"
#include "games/modulo/moduloverify.h"
#include "univalue/include/univalue.h"
#include "validation.h"
#include "wallet/coincontrol.h"
#include "wallet/wallet.h"

const std::string GAME_TAG = "303030303030";

void prepareTransaction(CMutableTransaction& txn_out)
{
    txn_out.nVersion = MAKE_MODULO_NEW_GAME_INDICATOR ^ CTransaction::CURRENT_VERSION;
    txn_out.vin.resize(1);
    txn_out.vin[0].prevout.n = 1;
    txn_out.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    txn_out.vout.resize(1);
}

std::string toHex(const std::string& s)
{
    std::ostringstream ret;

    for (std::string::size_type i = 0; i < s.length(); ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << (int)s[i];

    return ret.str();
}

BOOST_FIXTURE_TEST_SUITE(MakebetFormatTest, TestingSetup)

BOOST_AUTO_TEST_CASE(MakebetFormatTest_OpReturnExists)
{
    uint amount = 3;
    const std::string command = "24_straight_4@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_OpReturnNotExists)
{
    uint amount = 3;
    const std::string command = "24_straight_4@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_TxOutSizeToSmall)
{
    CMutableTransaction txn;
    prepareTransaction(txn);

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    uint amount = 30;
    const std::string command = "24_red@3000000000";

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_IncorrectDataLength)
{
    uint amount = 3;
    const std::string command = "24_straight_1@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    // modifi length val
    txn.vout[0].scriptPubKey[1] = 0x4d;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
    txn.vout[0].scriptPubKey[1] = 0x4e;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
    txn.vout[0].scriptPubKey[1] = 0x4c;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    // in this case length of data is on 1byte [2]
    txn.vout[0].scriptPubKey[1] = 0x4c;
    txn.vout[0].scriptPubKey[2] = 0x40;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_AmountBelowLimit)
{
    uint amount = 0;
    const std::string command = "24_straight_4@0+straight_1@0";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_ModuloArgAboveLimit)
{
    CAmount amount = 3;

    std::string game_tag;
    const std::string command = "_10@300000000";
    char max_reward[10];

    sprintf(max_reward, "%x", MAX_REWARD);
    game_tag = toHex(max_reward);

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    sprintf(max_reward, "%x", MAX_REWARD+1);
    game_tag = toHex(max_reward);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_UnrecognizedRouletteBetName)
{
    CAmount amount = 3;
    const std::string command = "24_dummy_4@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_MultipleBetsOneTransaction)
{
    CAmount amount = 3;
    const std::string command = "24_straight_4@300000000+straight_5@300000000+red@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = 3 * (amount * COIN);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_PlusSignAtEndOfTxnsCommand)
{
    CAmount amount = 3;
    const std::string command = "24_straight_4@300000000+straight_5@300000000+red@300000000+";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = 3 * (amount * COIN);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_LotteryGameBetZero)
{
    CAmount amount = 3;
    const std::string command = "10_1@300000000+2@300000000+0@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = 3 * (amount * COIN);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_RouletteGameBetZero)
{
    uint amount = 3;
    std::string command;

    CMutableTransaction txn;
    prepareTransaction(txn);

    command = "24_straight_0@300000000";
    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_split_0@300000000";
    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_line_0@300000000";
    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_0@300000000";
    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_01@300000000";
    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_1@300000000+line_0@300000000";
    txn.vout[0].nValue = 2 * (amount * COIN);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_ModuloBetNumberOverLimit)
{
    CAmount amount = 3;
    const std::string correct_command = "10_1@300000000+16@300000000";
    const std::string abovelimit_command = "10_1@300000000+17@300000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = 2 * (amount * COIN);
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(correct_command));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(abovelimit_command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_DummyStringWithOpReturn)
{
    uint amount = 3;
    std::string command;

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout[0].nValue = amount * COIN;

    command = "very_dummy+string";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_++red";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24_++red@300000000";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "24";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "00_red@300000000+black@300000000";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    command = "00_1@300000000";
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_AmountMismatch)
{
    uint amount = 3+4+2+1;
    const std::string command = "24_1@300000000+2@400000000+1@200000000+1@100000000";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[0].nValue = amount * COIN;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    txn.vout[0].nValue -= 1;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));

    txn.vout[0].nValue += 2;
    BOOST_CHECK_EQUAL(false, modulo::ver_2::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetPotentialRewardTest_SingleBetOverBlockSubsidyLimit_SumOfBetsBelow90)
{
    CAmount amount = GetBlockSubsidy(1, Params().GetConsensus()) / 2;
    CAmount rewardSum = 0, betsSum;
    std::string amountStr = std::to_string(amount);

    const std::string game_tag = "3030303030303031";
    std::string command = "_straight_4@";
    command += amountStr;

    CAmount potentialReward = amount * 1;
    BOOST_ASSERT(potentialReward < MAX_PAYOFF);

    CMutableTransaction txn;
    prepareTransaction(txn);

    // limit
    txn.vout[0].nValue = amount;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
    BOOST_CHECK_EQUAL(potentialReward, rewardSum);

    // overlimit, but sum of bets is less than 90% of block subsidy
    rewardSum = 0, betsSum = 0;
    amount += 1;
    txn.vout[0].nValue = amount;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetPotentialRewardTest_MultipleBetOverBlockSubsidylimit)
{
    CAmount one_bet_limit = (GetBlockSubsidy(1, Params().GetConsensus()) / 2);
    CAmount amount = (one_bet_limit/4);
    CAmount rewardSum = 0, betsSum = 0;
    const std::string game_tag = "3030303030303031";
    std::stringstream command{};
    command << "_1@" << amount << "+2@" << amount << "+3@" << amount << "+4@" << amount;

    CAmount potentialReward = 0;

    CMutableTransaction txn;
    prepareTransaction(txn);

    potentialReward = amount * 4;

    BOOST_ASSERT(potentialReward < MAX_PAYOFF);

    // limit
    txn.vout[0].nValue = 4 * amount;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command.str()));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
    BOOST_CHECK_EQUAL(potentialReward, rewardSum);

    // overlimit
    command.str() = ("");
    command << "_1@" << amount << "+2@" << amount << "+3@" << amount << "+4@" << amount+1;

    txn.vout[0].nValue = 4 * amount + 1;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command.str()));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetPotentialRewardTest_MultipleBetsOverRewardLimit)
{
    CAmount one_bet_limit = 1024*1024;
    uint modulo = 10000000;
    const std::string game_tag = "3030393839363830"; // modulo 10000000
    std::stringstream command{};
    command << "_";
    CAmount rewardSum = 0, betsSum = 0;

    CMutableTransaction txn;
    prepareTransaction(txn);

    CAmount potentialReward = 0;
    for (uint i=0; true; ++i)
    {
        if ((potentialReward + (one_bet_limit * modulo)) > MAX_PAYOFF)
        {
            break;
        }
        potentialReward += (one_bet_limit * modulo);

        if (i > 0) command << "+";
        command << "1@" << one_bet_limit;
    }

    BOOST_ASSERT(potentialReward == MAX_PAYOFF);

    // limit
    txn.vout[0].nValue = potentialReward;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command.str()));
    BOOST_CHECK_EQUAL(true, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
    BOOST_CHECK_EQUAL(potentialReward, rewardSum);

    // overlimit
    command << "+1@1";
    txn.vout[0].nValue = potentialReward;
    txn.vout[0].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command.str()));
    BOOST_CHECK_EQUAL(false, modulo::ver_2::checkBetsPotentialReward(rewardSum, betsSum, CTransaction(txn), true));
}

BOOST_AUTO_TEST_SUITE_END()


