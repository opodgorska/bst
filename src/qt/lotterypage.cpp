// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QMessageBox>
#include <QFileDialog>
#include <qt/bitcoinunits.h>
#include <qt/lotterypage.h>
#include <qt/forms/ui_lotterypage.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/askpassphrasedialog.h>
#include <qt/storetxdialog.h>

#include <wallet/coincontrol.h>
#include <validation.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif



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

#include <QSettings>
#include <QButtonGroup>
#include <QListWidgetItem>


LotteryPage::LotteryPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LotteryPage),
    walletModel(0),
    changeAddress("")
{
    ui->setupUi(this);

    connect(ui->makeBetButton, SIGNAL(clicked()), this, SLOT(makeBet()));
    connect(ui->getBetButton, SIGNAL(clicked()), this, SLOT(getBet()));
}

LotteryPage::~LotteryPage()
{
    delete ui;
}

void LotteryPage::setModel(WalletModel *model)
{
    walletModel = model;
}

void LotteryPage::unlockWallet()
{
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void LotteryPage::makeBet()
{
    if(walletModel)
    {
        try
        {
            std::shared_ptr<CWallet> wallet = GetWallets()[0];
            if(wallet != nullptr)
            {
                CWallet* const pwallet=wallet.get();

                int betNumber = ui->betNumberLineEdit->text().toInt();

                if(betNumber < 0 || betNumber >= MAX_BET_REWARD)
                {
                    throw std::runtime_error(std::string("Bet number is out of range <0, ")+std::to_string(MAX_BET_REWARD)+std::string(">"));
                }
                const Consensus::Params& params = Params().GetConsensus();
                double blockSubsidy = static_cast<double>(GetBlockSubsidy(chainActive.Height(), params)/COIN);
                double betAmount = ui->amountLineEdit->text().toDouble();
                if(betAmount <= 0 || betAmount >= (ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy))
                {
                    throw std::runtime_error(std::string("Bet amount is out of range <0, half of block mining reward>"));
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
                unlockWallet();
                tx.signTx();
                std::string txid=tx.sendTx().get_str();

    QListWidgetItem *newItem = new QListWidgetItem;
    newItem->setText(QString::fromStdString(txid));
    ui->transactionListWidget->insertItem(0, newItem);

            }
            else
            {
                throw std::runtime_error(std::string("No wallet found"));
            }
        }
        catch(std::exception const& e)
        {
            QMessageBox msgBox;
            msgBox.setText(e.what());
            msgBox.exec();
        }
        catch(...)
        {
            QMessageBox msgBox;
            msgBox.setText("Unknown exception occured");
            msgBox.exec();
        }
    }
}

void LotteryPage::getBet()
{
    if(walletModel)
    {
        try
        {
            std::shared_ptr<CWallet> wallet = GetWallets()[0];
            if(wallet != nullptr)
            {
                CWallet* const pwallet=wallet.get();
                
                QListWidgetItem *item = ui->transactionListWidget->currentItem();
                QString qtxid = item->text();
                std::string txidIn = qtxid.toStdString();
                
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

                //std::string address = request.params[1].get_str();
                std::string address("36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB");
                
                UniValue sendTo(UniValue::VOBJ);
                sendTo.pushKV("address", address);
                sendTo.pushKV("amount", amount);

                GetBetTxs tx(pwallet, txIn, sendTo, prevTxBlockHash);
                EnsureWalletIsUnlocked(pwallet);
                tx.signTx();
                std::string txid=tx.sendTx().get_str();

                //return UniValue(UniValue::VSTR, txid);

            }
            else
            {
                throw std::runtime_error(std::string("No wallet found"));
            }
        }
        catch(std::exception const& e)
        {
            QMessageBox msgBox;
            msgBox.setText(e.what());
            msgBox.exec();
        }
        catch(...)
        {
            QMessageBox msgBox;
            msgBox.setText("Unknown exception occured");
            msgBox.exec();
        }
    }
}
