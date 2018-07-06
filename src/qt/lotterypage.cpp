// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QSettings>
#include <QButtonGroup>
#include <QIntValidator>
#include <QDoubleValidator>

#include <qt/bitcoinunits.h>
#include <qt/lotterypage.h>
#include <qt/forms/ui_lotterypage.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
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

static const std::array<int, 9> confTargets = { {2, 4, 6, 12, 24, 48, 144, 504, 1008} };
extern int getConfTargetForIndex(int index);
extern int getIndexForConfTarget(int target);

LotteryPage::LotteryPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LotteryPage),
    walletModel(0),
    changeAddress(""),
    selectedItem(nullptr),
    fFeeMinimized(true)
{
    ui->setupUi(this);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1); // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0); // recommended
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_PAY_TX_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    groupFee = new QButtonGroup(this);
	groupFee->addButton(ui->radioSmartFee);
	groupFee->addButton(ui->radioCustomFee);
    groupFee->setId(ui->radioSmartFee, 0);
    groupFee->setId(ui->radioCustomFee, 1);
    groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());

    ui->betNumberLineEdit->setValidator( new QIntValidator(0, MAX_BET_REWARD-1, this) );

    connect(ui->makeBetButton, SIGNAL(clicked()), this, SLOT(makeBet()));
    connect(ui->getBetButton, SIGNAL(clicked()), this, SLOT(getBet()));
    
    loadListFromFile(QString("bets.dat"));
}

LotteryPage::~LotteryPage()
{
    delete ui;
}

void LotteryPage::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void LotteryPage::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void LotteryPage::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void LotteryPage::updateFeeMinimizedLabel()
{
    if(!walletModel || !walletModel->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void LotteryPage::updateMinFeeLabel()
{
    if (walletModel && walletModel->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
            BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), walletModel->wallet().getRequiredFee(1000)) + "/kB")
        );
}

void LotteryPage::updateCoinControlState(CCoinControl& ctrl)
{
    if (ui->radioCustomFee->isChecked()) {
        ctrl.m_feerate = CFeeRate(ui->customFee->value());
    } else {
        ctrl.m_feerate.reset();
    }
    // Avoid using global defaults when sending money from the GUI
    // Either custom fee will be used or if not selected, the confirmation target from dropdown box
    ctrl.m_confirm_target = getConfTargetForIndex(ui->confTargetSelector->currentIndex());
    ctrl.m_signal_bip125_rbf = ui->optInRBF->isChecked();
}

void LotteryPage::updateSmartFeeLabel()
{
    if(!walletModel || !walletModel->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateCoinControlState(coin_control);
    coin_control.m_feerate.reset(); // Explicitly use only fee estimation rate for smart fee labels
    int returned_target;
    FeeReason reason;
    feeRate = CFeeRate(walletModel->wallet().getMinimumFee(1000, coin_control, &returned_target, &reason));

    ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");

    if (reason == FeeReason::FALLBACK) {
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
        ui->fallbackFeeWarningLabel->setVisible(true);
        int lightness = ui->fallbackFeeWarningLabel->palette().color(QPalette::WindowText).lightness();
        QColor warning_colour(255 - (lightness / 5), 176 - (lightness / 3), 48 - (lightness / 14));
        ui->fallbackFeeWarningLabel->setStyleSheet("QLabel { color: " + warning_colour.name() + "; }");
        ui->fallbackFeeWarningLabel->setIndent(QFontMetrics(ui->fallbackFeeWarningLabel->font()).width("x"));
    }
    else
    {
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", returned_target));
        ui->fallbackFeeWarningLabel->setVisible(false);
    }

    updateFeeMinimizedLabel();
}

void LotteryPage::setMinimumFee()
{
    ui->customFee->setValue(walletModel->wallet().getRequiredFee(1000));
}

void LotteryPage::updateFeeSectionControls()
{
    ui->confTargetSelector      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelCustomPerKilobyte  ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->customFee               ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

void LotteryPage::setBalance(const interfaces::WalletBalances& balances)
{
    if(walletModel && walletModel->getOptionsModel())
    {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}

void LotteryPage::updateDisplayUnit()
{
    setBalance(walletModel->wallet().getBalances());
    ui->customFee->setDisplayUnit(walletModel->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
    updateRewardView();
}

void LotteryPage::setModel(WalletModel *model)
{
    walletModel = model;

    interfaces::WalletBalances balances = walletModel->wallet().getBalances();
    setBalance(balances);
    connect(walletModel, SIGNAL(balanceChanged(interfaces::WalletBalances)), this, SLOT(setBalance(interfaces::WalletBalances)));
    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    for (const int n : confTargets) {
        ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().nPowTargetSpacing)).arg(n));
    }
    connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSmartFeeLabel()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
    connect(groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
    connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
    connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(updateSmartFeeLabel()));
    ui->customFee->setSingleStep(model->wallet().getRequiredFee(1000));
    updateFeeSectionControls();
    updateMinFeeLabel();
    updateSmartFeeLabel();
    
    // set default rbf checkbox state
    ui->optInRBF->setCheckState(Qt::Checked);

    // set the smartfee-sliders default value (wallets default conf.target or last stored value)
    QSettings settings;
    if (settings.value("nSmartFeeSliderPosition").toInt() != 0) {
        // migrate nSmartFeeSliderPosition to nConfTarget
        // nConfTarget is available since 0.15 (replaced nSmartFeeSliderPosition)
        int nConfirmTarget = 25 - settings.value("nSmartFeeSliderPosition").toInt(); // 25 == old slider range
        settings.setValue("nConfTarget", nConfirmTarget);
        settings.remove("nSmartFeeSliderPosition");
    }
    if (settings.value("nConfTarget").toInt() == 0)
        ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(model->wallet().getConfirmTarget()));
    else
        ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(settings.value("nConfTarget").toInt()));

    connect(ui->betNumberLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateRewardView()));
    connect(ui->amountLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateRewardView()));
    ui->maxRewardValLabel->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), 0));
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

void LotteryPage::dumpListToFile(const QString& fileName)
{
    fs::path dataPath = GetDataDir();

    if (!fs::exists(dataPath))
    {
        throw std::runtime_error(std::string("Data directory doesn't exist"));
    }
    QString path=GUIUtil::boostPathToQString(dataPath)+QString("/")+fileName;

    QFile dumpFile(path);
    dumpFile.open(QIODevice::WriteOnly | QIODevice::Text);
    if(!dumpFile.isOpen())
    {
        throw std::runtime_error(std::string("Data directory doesn't exist"));
    }
    QTextStream outStream(&dumpFile);
    for(int i = 0; i < ui->transactionListWidget->count(); ++i)
    {
        QString txid = ui->transactionListWidget->item(i)->text();
        outStream << txid << QString("\n");
    }
    dumpFile.close();
}

void LotteryPage::loadListFromFile(const QString& fileName)
{
    fs::path dataPath = GetDataDir();

    if (!fs::exists(dataPath))
    {
        throw std::runtime_error(std::string("Data directory doesn't exist"));
    }
    QString path=GUIUtil::boostPathToQString(dataPath)+QString("/")+fileName;

    QFile inputFile(path);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString txid = in.readLine();
            QListWidgetItem *newItem = new QListWidgetItem;
            newItem->setText(txid.left(64));
            ui->transactionListWidget->insertItem(0, newItem);
        }
        inputFile.close();
    }
}

void LotteryPage::updateRewardView()
{
    int betNumber = ui->betNumberLineEdit->text().toInt();
    int mask = getMask(betNumber);
    double betAmount = ui->amountLineEdit->text().toDouble();
    double reward = maskToReward(mask)*betAmount;
    if(ui->amountLineEdit->text().length()==0 || ui->betNumberLineEdit->text().length()==0)
    {
        reward = 0.0;
    }
    if(walletModel && walletModel->getOptionsModel())
    {
        ui->maxRewardValLabel->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), static_cast<CAmount>(reward*COIN)));
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

                if(ui->betNumberLineEdit->text().length()==0)
                {
                    throw std::runtime_error(std::string("Bet number must be provided"));
                }
                if(ui->amountLineEdit->text().length()==0)
                {
                    throw std::runtime_error(std::string("Amount must be provided"));
                }

                if(betNumber < 0 || betNumber >= MAX_BET_REWARD)
                {
                    throw std::runtime_error(std::string("Bet number is out of range <0, ")+std::to_string(MAX_BET_REWARD)+std::string(">"));
                }
                const Consensus::Params& params = Params().GetConsensus();
                double blockSubsidy = static_cast<double>(GetBlockSubsidy(chainActive.Height(), params)/COIN);
                double betAmount = ui->amountLineEdit->text().toDouble();
                if(betAmount <= 0 || betAmount >= (ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy))
                {
                    throw std::runtime_error(std::string("Amount is out of range <0, ")+std::to_string(ACCUMULATED_BET_REWARD_FOR_BLOCK*blockSubsidy)+std::string(">"));
                }
                int mask = getMask(betNumber);
                constexpr size_t txSize=265;
                double fee;

                CCoinControl coin_control;
                updateCoinControlState(coin_control);
                int returned_target;
                FeeReason reason;
                CFeeRate feeRate = CFeeRate(walletModel->wallet().getMinimumFee(1000, coin_control, &returned_target, &reason));

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

                MakeBetTxs tx(pwallet, inputs, sendTo, 0, ui->optInRBF->isChecked());
                unlockWallet();
                tx.signTx();
                std::string txid=tx.sendTx().get_str();

                QListWidgetItem *newItem = new QListWidgetItem;
                newItem->setText(QString::fromStdString(txid));
                ui->transactionListWidget->insertItem(0, newItem);

                dumpListToFile(QString("bets.dat"));
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
                
                selectedItem = ui->transactionListWidget->currentItem();
                if(selectedItem==nullptr)
                {
                    throw std::runtime_error(std::string("No transaction selected"));
                }
                QString qtxid = selectedItem->text();
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
                updateCoinControlState(coin_control);
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

                std::string address = ui->addressLineEdit->text().toStdString();
                
                UniValue sendTo(UniValue::VOBJ);
                sendTo.pushKV("address", address);
                sendTo.pushKV("amount", amount);

                GetBetTxs tx(pwallet, txIn, sendTo, prevTxBlockHash, 0, ui->optInRBF->isChecked());
                unlockWallet();
                tx.signTx();
                std::string txid=tx.sendTx().get_str();

                QMessageBox msgBox;
                msgBox.setText(QString::fromStdString(txid));
                msgBox.exec();

                delete ui->transactionListWidget->takeItem(ui->transactionListWidget->row(selectedItem));
                dumpListToFile(QString("bets.dat"));
            }
            else
            {
                throw std::runtime_error(std::string("No wallet found"));
            }
        }
        catch(std::exception const& e)
        {
            std::string info=e.what();
            if(info==std::string("Script failed an OP_EQUALVERIFY operation") || 
               info==std::string("Input not found or already spent"))
            {
                delete ui->transactionListWidget->takeItem(ui->transactionListWidget->row(selectedItem));
                dumpListToFile(QString("bets.dat"));
            }
            
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
