#include <test/test_bitcoin.h>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>

#include "games/modulo/moduloverify.h"
#include "games/modulo/modulotxs.h"
#include "games/modulo/moduloutils.h"
#include "rpc/rawtransaction.h"

namespace {
std::string toHex(const std::string& s)
{
    std::ostringstream ret;

    for (std::string::size_type i = 0; i < s.length(); ++i)
        ret << std::hex << std::setfill('0') << std::setw(2) << (int)s[i];

    return ret.str();
}

void preparePrevTransaction(CMutableTransaction& txn)
{
    txn.nVersion = MAKE_MODULO_GAME_INDICATOR ^ CTransaction::CURRENT_VERSION;
    txn.vin.resize(1);
    txn.vin[0].prevout.n = 1;
    txn.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    txn.vout.resize(4);
    txn.vout[0].nValue = 100000000;
    txn.vout[0].scriptPubKey = CScript() << OP_HASH160 << ParseHex("5B38014AAA19E0A9565A42BFDEEFEF3FCE954D7A") << OP_EQUAL;
    txn.vout[1].nValue = 100000000;
    txn.vout[1].scriptPubKey = CScript() << OP_HASH160 << ParseHex("1B860F51C90D0D47FCD316CC10AE989828F531EB") << OP_EQUAL;
//    txn.vout[2].nValue = 0;
//    txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));
    txn.vout[3].nValue = 99982580;
    txn.vout[3].scriptPubKey = CScript() << OP_HASH160 << ParseHex("8A9F0820EB01514EB492CD55B2DF744CEB344CD5") << OP_EQUAL;
}

void prepareTransaction(CMutableTransaction& txn)
{
    unsigned char hexHash[32] = {228,100,107,103,52,223,3,128,98,231,15,235,51,124,220,225,10,154,112,155,15,124,164,6,213,69,116,8,165,24,42,20};

    txn.nVersion = 2;
    txn.vin.resize(1);
    txn.vin[0].prevout.hash.SetHex((char*)hexHash);
    txn.vin[0].prevout.n = 0;
    txn.vin[0].nSequence = 1;
}

bool run_txVerify(CMutableTransaction& prev_txn, UniValue& uv, CMutableTransaction& txn)
{
    modulo::ModuloOperation operation;
    modulo::GetModuloReward getReward;
    modulo::CompareModuloBet2Vector compareBet2Vector;


    CAmount totalReward{}, inputSum{};
    int nSpendHeight = 10;

    unsigned int idx = 0;

    bool rv = txVerify(nSpendHeight,
             CTransaction(txn),
             MakeTransactionRef(prev_txn),
             uv,
             &operation,
             &getReward,
             &compareBet2Vector,
             MAKE_MODULO_GAME_INDICATOR,
             MAX_REWARD,
             totalReward,
             inputSum,
             idx);
    return rv;
}

// red game, length 0x4c
const std::string& red_game   = "76a91453aeee3febe3f8828bfa7a300075a920416645c688ac637604010000008763755167760403000000876375516776040500000087637551677604070000008763755167760409000000876375516776040c000000876375516776040e000000876375516776041000000087637551677604120000008763755167760413000000876375516776041500000087637551677604170000008763755167760419000000876375516776041b000000876375516776041e00000087637551677604200000008763755167760422000000876375516704240000008851686868686868686868686868686868686867750068042400000075";
// black game, length 0x4c
const std::string& black_game = "76a9140574b4cb35486c6849a8c1f3d14b9a00801155ae88ac63760402000000876375516776040400000087637551677604060000008763755167760408000000876375516776040a000000876375516776040b000000876375516776040d000000876375516776040f0000008763755167760411000000876375516776041400000087637551677604160000008763755167760418000000876375516776041a000000876375516776041c000000876375516776041d000000876375516776041f0000008763755167760421000000876375516704230000008851686868686868686868686868686868686867750068042400000075";

//straight_2 game, length 0x2b
const std::string& straight2_game = "76a9144d722e96b345aa2cd394133a53efff804a34fe3e88ac630402000000885167750068042400000075";
}

BOOST_FIXTURE_TEST_SUITE(GetBetTest, TestingSetup)

BOOST_AUTO_TEST_CASE(GetBetTestFormat_TxnCorrectRed)
{

    const std::string command = "00000000000024_red";

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304") // idk
                                     << ParseHex("0A0B0C0D") // idk
                                     << ParseHex(red_game);
    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_TxnIncorrectVersion)
{

    const std::string command = "00000000000024_black";

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D")
                                     << ParseHex(black_game);
    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    UV_txPrev.pushKV("version", MAKE_MODULO_GAME_INDICATOR-1);

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_BlockhashMismatch)
{

    const std::string command = "00000000000024_red";

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("14000000") // blockhash 20 <> 21
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D")
                                     << ParseHex(red_game);
    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_TxnOrder4cIncorrectLength)
{
    // order 0x4C is special case that needs to have one additional byte for length

    const std::string command = "00000000000024_red";
    CScript game = CScript() << ParseHex(red_game);
    game.erase(game.begin()+1); // remove additional byte of length

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_TxnCorrectStraight2)
{

    const std::string command = "00000000000024_straight_2";

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D")
                                     << ParseHex(straight2_game);
    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_TxnTooShortCommand)
{
    const std::string command = "00000000000024_straight_2";

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].prevout.n = 2; // fail because it's only one game in command
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D")
                                     << ParseHex(straight2_game);
    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_Missing_OPHash160)
{
    const std::string command = "00000000000024_straight_2";
    CScript game = CScript() << ParseHex(straight2_game);
    game[1] = 0x00; //missing OP_HASH160

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_MissingOPEqualVerify)
{
    const std::string command = "00000000000024_straight_2";
    // 76a9144d722e96b345aa2cd394133a53efff804a34fe3e88ac630402000000885167750068042400000075
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = std::find(game.begin(), game.end(), OP_EQUALVERIFY);
    (*it) = 0x00;

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));

    (*it) = OP_EQUALVERIFY;

    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));

    it = std::find(it+1, game.end(), OP_EQUALVERIFY);
    (*it) = 0x00;

    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_MissingOPDrop)
{
    const std::string command = "00000000000024_straight_2";
    // 76a9144d722e96b345aa2cd394133a53efff804a34fe3e88ac630402000000885167750068042400000075
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = game.end() - 1;
    (*it) = 0x00;

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));

    (*it) = OP_DROP;
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));

    it -= 8;
    (*it) = 0x00;
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_Missing_OPEndif)
{
    const std::string command = "00000000000024_straight_2";
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = game.end() - 7;
    (*it) = 0x00; //missing OP_ENDIF

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_Missing_OPFalse)
{
    const std::string command = "00000000000024_straight_2";
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = game.end() - 8;
    (*it) = 0xFF; //missing OP_FALSE

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_Missing_OPElse)
{
    const std::string command = "00000000000024_straight_2";
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = game.end() - 10;
    (*it) = 0x00; //missing OP_ELSE

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_Missing_OPTrue)
{
    const std::string command = "00000000000024_straight_2";
    CScript game = CScript() << ParseHex(straight2_game);
    CScript::iterator it = game.end() - 11;
    (*it) = 0x00; //missing OP_TRUE

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_BetNumbersFormat)
{

    const std::string command = "00000000000024_red";
    CScript game = CScript() << ParseHex(red_game);

    CMutableTransaction prev_txn;
    preparePrevTransaction(prev_txn);
    prev_txn.vout[2].nValue = 0;
    prev_txn.vout[2].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    CAmount amount = 5;
    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    txn.vout.resize(2);
    txn.vout[1].nValue = amount * COIN;
    txn.vout[1].scriptPubKey = CScript() << OP_RETURN << ParseHex(toHex(command));

    UniValue UV_txPrev(UniValue::VOBJ);
    TxToJSON(CTransaction(prev_txn), txn.vin[0].prevout.hash, UV_txPrev);
    UV_txPrev.pushKV("in_active_chain", 0);
    UV_txPrev.pushKV("blockhash", "80000000c33b1ab39daba5e67b6be2b28b59b32cf5edd14c138713665e068db8");

    CScript::iterator it = std::find(game.begin() + 4, game.end(), OP_DUP);
    for (int i=0; i<(6 * 17); ++it, ++i)
    {
        bool byteCheck = ((*it)==OP_DUP || (*it)==OP_EQUAL || (*it)==OP_IF || (*it)==OP_DROP || (*it)==OP_TRUE || (*it)==OP_ELSE);
        BOOST_ASSERT(byteCheck);
        txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                         << ParseHex("01020304")
                                         << ParseHex("0A0B0C0D");
        txn.vin[0].scriptSig += game;
        BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));

        unsigned char prevVal = (*it);
        (*it) = 0x00;
        txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                         << ParseHex("01020304")
                                         << ParseHex("0A0B0C0D");
        txn.vin[0].scriptSig += game;

        BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));

        (*it) = prevVal;
        if (*(it) == OP_DUP) it += 5; // skip number array, always after OP_DUP
    }

    it += 5; // skip number array (last number)
    BOOST_ASSERT(*(it) == OP_EQUALVERIFY);

    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;
    BOOST_CHECK_EQUAL(true, run_txVerify(prev_txn, UV_txPrev, txn));

    (*it) = 0x00;
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(false, run_txVerify(prev_txn, UV_txPrev, txn));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_isInputBet)
{

    const std::string command = "00000000000024_red";
    CScript game = CScript() << ParseHex(red_game);

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    CScript::iterator it = std::find(game.begin() + 4, game.end(), OP_DUP);
    for (int i=0; i<(6 * 17); ++it, ++i)
    {
        bool byteCheck = ((*it)==OP_DUP || (*it)==OP_EQUAL || (*it)==OP_IF || (*it)==OP_DROP || (*it)==OP_TRUE || (*it)==OP_ELSE);
        BOOST_ASSERT(byteCheck);
        txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                         << ParseHex("01020304")
                                         << ParseHex("0A0B0C0D");
        txn.vin[0].scriptSig += game;
        BOOST_CHECK_EQUAL(true, isInputBet(txn.vin[0]));

        unsigned char prevVal = (*it);
        (*it) = 0x00;
        txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                         << ParseHex("01020304")
                                         << ParseHex("0A0B0C0D");
        txn.vin[0].scriptSig += game;

        BOOST_CHECK_EQUAL(false, isInputBet(txn.vin[0]));

        (*it) = prevVal;
        if (*(it) == OP_DUP) it += 5; // skip number array, always after OP_DUP
    }

    it += 5; // skip number array (last number)
    BOOST_ASSERT(*(it) == OP_EQUALVERIFY);

    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;
    BOOST_CHECK_EQUAL(true, isInputBet(txn.vin[0]));

    (*it) = 0x00;
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    BOOST_CHECK_EQUAL(false, isInputBet(txn.vin[0]));
}

BOOST_AUTO_TEST_CASE(GetBetTestFormat_isInputBetWithAdditionalArgs)
{
    std::vector<int> red_game_values(18);
    for (uint i=0; i<red_game_values.size(); ++i)
    {
        red_game_values[i] = modulo::red[i];
    }


    const std::string command = "00000000000024_red";
    CScript game = CScript() << ParseHex(red_game);

    CMutableTransaction txn;
    prepareTransaction(txn);
    txn.vin[0].scriptSig = CScript() << ParseHex("15000000") // 21 blockhash
                                     << ParseHex("01020304")
                                     << ParseHex("0A0B0C0D");
    txn.vin[0].scriptSig += game;

    uint _numOfBets = 0;
    std::vector<int> _betNumbers;

    BOOST_CHECK_EQUAL(true, isInputBet(txn.vin[0], &_numOfBets, &_betNumbers));
    BOOST_CHECK_EQUAL(_numOfBets, red_game_values.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(_betNumbers.rbegin(), _betNumbers.rend(), red_game_values.begin(), red_game_values.end());
}

BOOST_AUTO_TEST_SUITE_END()
