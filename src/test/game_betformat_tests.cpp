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
    txn_out.nVersion = MAKE_MODULO_GAME_INDICATOR ^ CTransaction::CURRENT_VERSION;
    txn_out.vin.resize(1);
    txn_out.vin[0].prevout.n = 1;
    txn_out.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    txn_out.vout.resize(2);
    txn_out.vout[0].nValue = 1;
    txn_out.vout[0].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
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
    const std::string command = "24_straight_4";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_OpReturnNotExists)
{
    uint amount = 3;
    const std::string command = "24_straight_4";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_TxOutSizeToSmall)
{
    CMutableTransaction txn;
    prepareTransaction(txn);

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    uint amount = 30;
    const std::string command = "24_red";

    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_IncorrectDataLength)
{
    uint amount = 3;
    const std::string command = "24_straight_1";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));

    // modifi length val
    txn.vout[1].scriptPubKey[1] = 0x4d;
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
    txn.vout[1].scriptPubKey[1] = 0x4e;
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
    txn.vout[1].scriptPubKey[1] = 0x4c;
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    // in this case length of data is on 1byte [2]
    txn.vout[1].scriptPubKey[1] = 0x4c;
    txn.vout[1].scriptPubKey[2] = 0x40;
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_AmountBelowLimit)
{
    uint amount = 0;
    const std::string command = "24_straight_4+straight_1";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_ModuloArgAboveLimit)
{
    CAmount amount = 5;

    std::string game_tag;
    const std::string command = "_10";
    char max_reward[10];

    sprintf(max_reward, "%x", MAX_REWARD);
    game_tag = toHex(max_reward);

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));

    sprintf(max_reward, "%x", MAX_REWARD+1);
    game_tag = toHex(max_reward);
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

}


BOOST_AUTO_TEST_CASE(MakebetFormatTest_UnrecognizedRouletteBetName)
{
    uint amount = 3;
    const std::string command = "24_dummy_4";

    CMutableTransaction txn;
    prepareTransaction(txn);

    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_MultipleBetsOneTransaction)
{
    const std::string command = "24_straight_4+straight_5+red";

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout.resize(5);

    for (uint i=0; i<3; ++i)
    {
        txn.vout[i].nValue = (i+2) * COIN;
        txn.vout[i].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
    }

    txn.vout[3].nValue = 0;
    txn.vout[3].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_PlusSignAtEndOfTxnsCommand)
{
    const std::string command = "24_straight_4+straight_5+red+";

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout.resize(5);

    for (uint i=0; i<3; ++i)
    {
        txn.vout[i].nValue = (i+2) * COIN;
        txn.vout[i].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
    }

    txn.vout[3].nValue = 0;
    txn.vout[3].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_LotteryGameBetZero)
{
    const std::string command = "10_1+2+0";

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout.resize(5);

    for (uint i=0; i<3; ++i)
    {
        txn.vout[i].nValue = (i+2) * COIN;
        txn.vout[i].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
    }

    txn.vout[3].nValue = 0;
    txn.vout[3].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));

    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_RouletteGameBetZero)
{
    uint amount = 3;
    std::string command;

    CMutableTransaction txn;
    prepareTransaction(txn);

    command = "24_straight_0";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_straight_0";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_line_0";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_0";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_01";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_column_1+line_0";
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_ModuloBetNumberOverLimit)
{
    const std::string correct_command = "10_1+16";
    const std::string abovelimit_command = "10_1+17";

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout.resize(4);

    txn.vout[1].nValue = 2 * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
    txn.vout[2].nValue = 0;
    txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(correct_command));

    BOOST_CHECK_EQUAL(true, modulo::txMakeBetVerify(CTransaction(txn), true));

    txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(abovelimit_command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

}

BOOST_AUTO_TEST_CASE(MakebetFormatTest_DummyStringWithOpReturn)
{
    uint amount = 3;
    std::string command;

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout[1].nValue = amount * COIN;

    command = "very_dummy+string";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_++red";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24_++red";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "24";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "00_red+black";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));

    command = "00_1";
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::txMakeBetVerify(CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetPotentialRewardTest_SingleBetOverBlockSubsidyLimit)
{
    CAmount amount = GetBlockSubsidy(1, Params().GetConsensus()) / 2;
    CAmount rewardSum = 0;
    const std::string game_tag = "3030303030303031";
    const std::string command = "_straight_4";

    CAmount potentialReward = amount * 1;
    BOOST_ASSERT(potentialReward < MAX_PAYOFF);

    CMutableTransaction txn;
    prepareTransaction(txn);

    // limit (50% of block subsidy)
    txn.vout[0].nValue = amount;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::checkBetsPotentialReward(rewardSum, CTransaction(txn), true));
    BOOST_CHECK_EQUAL(potentialReward, rewardSum);

    // overlimit
    rewardSum = 0;
    amount += 1;
    txn.vout[0].nValue = amount;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::checkBetsPotentialReward(rewardSum, CTransaction(txn), true));
}

BOOST_AUTO_TEST_CASE(MakebetPotentialRewardTest_MultipleBetOverBlockSubsidylimit)
{
    CAmount one_bet_limit = (GetBlockSubsidy(1, Params().GetConsensus()) / 2);
    CAmount amount = (one_bet_limit/4);
    CAmount rewardSum = 0;
    const std::string game_tag = "3030303030303031";
    const std::string command = "_1+2+3+4";

    CAmount potentialReward = 0;

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vout.resize(5);

    for (uint i=0; i<4; ++i)
    {
        txn.vout[i].nValue = amount;
        txn.vout[i].scriptPubKey = CScript() << OP_HASH160 << ParseHex("0102030405060708090A0B0C0D0E0F1011121314") << OP_EQUAL;
        potentialReward += (amount * 1);
    }
    BOOST_ASSERT(potentialReward < MAX_PAYOFF);

    // limit (50% of block subsidy)
    txn.vout[4].scriptPubKey = CScript() << OP_RETURN << ParseHex(game_tag + toHex(command));
    BOOST_CHECK_EQUAL(true, modulo::checkBetsPotentialReward(rewardSum, CTransaction(txn), true));
    BOOST_CHECK_EQUAL(potentialReward, rewardSum);

    // overlimit
    amount += 1;
    txn.vout[3].nValue = amount;
    txn.vout[4].scriptPubKey = CScript() << OP_RETURN << ParseHex(GAME_TAG + toHex(command));
    BOOST_CHECK_EQUAL(false, modulo::checkBetsPotentialReward(rewardSum, CTransaction(txn), true));
}


BOOST_AUTO_TEST_SUITE_END()


