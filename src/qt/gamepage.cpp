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

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/gamepage.h>
#include <qt/forms/ui_gamepage.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/askpassphrasedialog.h>
#include <qt/storetxdialog.h>

#include <consensus/validation.h>
#include <wallet/coincontrol.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <validation.h>
#include <policy/policy.h>
#include <utilstrencodings.h>
#include <stdint.h>
#include <amount.h>
#include <chainparams.h>
#include <net.h>
#include <utilmoneystr.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <univalue.h>
#include <boost/algorithm/string.hpp>

#include <data/datautils.h>
#include <data/processunspent.h>
#include <games/gamestxs.h>
#include <games/gamesutils.h>
#include <games/modulo/modulotxs.h>
#include <games/modulo/moduloutils.h>
#include <games/modulo/moduloverify.h>

#include <qt/transactiontablemodel.h>
#include <array>


using namespace modulo;

static const std::array<QString, 13> betTypes = { {QString("Straight"), QString("Split"), QString("Street"), 
                                                  QString("Corner"), QString("Line"), QString("Column"), 
                                                  QString("Dozen"), QString("Low"), QString("High"),
                                                  QString("Even"), QString("Odd"), QString("Red"), QString("Black")} };

static const std::array<int, 9> confTargets = { {2, 4, 6, 12, 24, 48, 144, 504, 1008} };
extern int getConfTargetForIndex(int index);
extern int getIndexForConfTarget(int target);

GamePage::GamePage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GamePage),
    walletModel(0),
    clientModel(0),
    changeAddress(""),
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

    ui->betNumberSpinBox->setMinimum(1);
    ui->rewardRatioSpinBox->setMinimum(2);
    ui->rewardRatioSpinBox->setMaximum(MAX_REWARD);
    ui->amountSpinBox->setDecimals(8);
    ui->amountSpinBox->setMaximum(MAX_PAYOFF/COIN);

    {
        std::lock_guard<std::mutex> lck(mtx);
        loadListFromFile(QString("bets.dat"));
    }
}

GamePage::~GamePage()
{
    delete ui;
}

void GamePage::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void GamePage::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void GamePage::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void GamePage::updateFeeMinimizedLabel()
{
    if(!walletModel || !walletModel->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void GamePage::updateMinFeeLabel()
{
    if (walletModel && walletModel->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
            BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), walletModel->wallet().getRequiredFee(1000)) + "/kB")
        );
}

void GamePage::updateCoinControlState(CCoinControl& ctrl)
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

void GamePage::updateSmartFeeLabel()
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

void GamePage::setMinimumFee()
{
    ui->customFee->setValue(walletModel->wallet().getRequiredFee(1000));
}

void GamePage::updateFeeSectionControls()
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

void GamePage::setBalance(const interfaces::WalletBalances& balances)
{
    if(walletModel && walletModel->getOptionsModel())
    {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}

void GamePage::updateDisplayUnit()
{
    setBalance(walletModel->wallet().getBalances());
    ui->customFee->setDisplayUnit(walletModel->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
    updateRewardView();
}

void GamePage::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void GamePage::setModel(WalletModel *model)
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

    ui->gameTypeComboBox->addItem(QString("Roulette"));
    ui->gameTypeComboBox->addItem(QString("Lottery"));

    ui->rewardRatioSpinBox->setValue(36);
    ui->rewardRatioSpinBox->setEnabled(false);

    for(size_t i=0; i<betTypes.size(); ++i)
    {
        ui->betTypeComboBox->addItem(betTypes[i]);
    }
    ui->betTypeComboBox->setEnabled(true);

    connect(ui->gameTypeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateGameType()));
    connect(ui->betTypeComboBox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updateBetType()));
    connect(ui->rewardRatioSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateRewardView()));
    connect(ui->betListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateRewardView()));
    connect(ui->addBetButton, SIGNAL(clicked()), this, SLOT(addBet()));
    connect(ui->deleteBetButton, SIGNAL(clicked()), this, SLOT(deleteBet()));
    connect(ui->makeBetButton, SIGNAL(clicked()), this, SLOT(makeBet()));
    connect(ui->getBetButton, SIGNAL(clicked()), this, SLOT(getBet()));
    ui->maxRewardValLabel->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), 0));

    connect(walletModel->getTransactionTableModel(), SIGNAL(newTx(const QString& )), this, SLOT(newTx(const QString& )));
    connect(walletModel->getTransactionTableModel(), SIGNAL(deletedTx(const QString& )), this, SLOT(deletedTx(const QString& )));

    updateBetNumberLimit();
}

void GamePage::deletedTx(const QString &hash)
{
    std::lock_guard<std::mutex> lck(mtx);
    if(removeTxidFromList(hash))
    {
        dumpListToFile(QString("bets.dat"));
    }
}

void GamePage::newTx(const QString &hash)
{
    std::lock_guard<std::mutex> lck(mtx);
    if(addTxidToList(hash))
    {
        dumpListToFile(QString("bets.dat"));
    }
}


void GamePage::clearGameTypeBox()
{
    int count = ui->betListWidget->count();
    for(int i=0;i<count;++i)
    {
        delete ui->betListWidget->takeItem(0);
    }
}

void GamePage::updateBetType()
{
    if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
    {
        if(ui->betTypeComboBox->currentIndex() >= 7)
        {
            ui->betNumberSpinBox->setEnabled(false);
        }
        else
        {
            ui->betNumberSpinBox->setEnabled(true);
        }
    }
    else if(ui->gameTypeComboBox->currentIndex() == 1)//lottery
    {
        ui->betNumberSpinBox->setEnabled(true);
    }
    updateBetNumberLimit();
}

void GamePage::updateGameType()
{
    clearGameTypeBox();
    if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
    {
        ui->rewardRatioSpinBox->setValue(36);
        ui->rewardRatioSpinBox->setEnabled(false);
        ui->betTypeComboBox->setEnabled(true);
    }
    else if(ui->gameTypeComboBox->currentIndex() == 1)//lottery
    {
        ui->rewardRatioSpinBox->setEnabled(true);
        ui->betTypeComboBox->setCurrentIndex(0);
        ui->betTypeComboBox->setEnabled(false);
    }
    updateRewardView();
    updateBetNumberLimit();
}

void GamePage::updateBetNumberLimit()
{
    if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
    {
        if(ui->betTypeComboBox->currentText()==QString("Straight"))
        {
            ui->betNumberSpinBox->setMaximum(ui->rewardRatioSpinBox->value());
        }
        else if(ui->betTypeComboBox->currentText()==QString("Split"))
        {
            ui->betNumberSpinBox->setMaximum(splitBetsNum);
        }
        else if(ui->betTypeComboBox->currentText()==QString("Street"))
        {
            ui->betNumberSpinBox->setMaximum(streetBetsNum);
        }
        else if(ui->betTypeComboBox->currentText()==QString("Corner"))
        {
            ui->betNumberSpinBox->setMaximum(cornerBetsNum);
        }
        else if(ui->betTypeComboBox->currentText()==QString("Line"))
        {
            ui->betNumberSpinBox->setMaximum(lineBetsNum);
        }
        else if(ui->betTypeComboBox->currentText()==QString("Column"))
        {
            ui->betNumberSpinBox->setMaximum(columnBetsNum);
        }
        else if(ui->betTypeComboBox->currentText()==QString("Dozen"))
        {
            ui->betNumberSpinBox->setMaximum(dozenBetsNum);
        }
    }
    else if(ui->gameTypeComboBox->currentIndex() == 1)//lottery
    {
        ui->betNumberSpinBox->setMaximum(ui->rewardRatioSpinBox->value());
    }
}

void GamePage::deleteBet()
{
    QListWidgetItem *it;
    it = ui->betListWidget->currentItem();
    delete ui->betListWidget->takeItem(ui->betListWidget->row(it));

    updateRewardView();
}

void GamePage::addBet()
{
    if(ui->betNumberSpinBox->value() > ui->rewardRatioSpinBox->value())
    {
        QMessageBox msgBox;
        msgBox.setText("Bet number is greater than reward ratio");
        msgBox.exec();
        return;
    }

    QString betString;
    double amount=ui->amountSpinBox->value();
    if(amount<=0.0)
    {
        QMessageBox msgBox;
        msgBox.setText("Amount is out of range");
        msgBox.exec();
        return;
    }
    if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
    {
        betString+=ui->betTypeComboBox->currentText().toLower();
        if(ui->betTypeComboBox->currentIndex() < 7)
        {
            betString+=QString("_");
            betString+=QString::number(ui->betNumberSpinBox->value());
        }
        betString+=QString("@");
        betString+=QString::number(amount, 'f', 8);

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(betString);
        ui->betListWidget->insertItem(0, newItem);
    }
    else if(ui->gameTypeComboBox->currentIndex() == 1)//lottery
    {
        betString+=QString::number(ui->betNumberSpinBox->value());
        betString+=QString("@");
        betString+=QString::number(amount, 'f', 8);

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(betString);
        ui->betListWidget->insertItem(0, newItem);
    }

    updateRewardView();
}

void GamePage::updateRewardView()
{
    int reward;
    double betAmount;
    int* betArray;
    int betLen;
    std::string betType;
    double payoff=0.0;
    
    QListWidgetItem *it;
    it = ui->betListWidget->currentItem();
    if(it!=nullptr)
    {
        std::string betTypePattern=it->text().toStdString();
        if(!betTypePattern.empty())
        {
            try
            {
                if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
                {
                    int range=36;
                    getRouletteBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
                }
                else if(ui->gameTypeComboBox->currentIndex() == 1)//lottery
                {
                    int range=ui->rewardRatioSpinBox->value();
                    getModuloBet(betTypePattern, betArray, betLen, reward, betAmount, betType, range);
                }
            }
            catch(std::exception const& e)
            {
                QMessageBox msgBox;
                msgBox.setText(e.what());
                msgBox.exec();
            }
            payoff=(betAmount*reward);
        }
    }

    if(walletModel && walletModel->getOptionsModel())
    {
        ui->maxRewardValLabel->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), static_cast<CAmount>(payoff*COIN)));
    }
    updateBetNumberLimit();
}

void GamePage::unlockWallet()
{
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void GamePage::dumpListToFile(const QString& fileName)
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

void GamePage::loadListFromFile(const QString& fileName)
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

bool GamePage::isTxidInList(const QString& txid)
{
    for(int i = 0; i < ui->transactionListWidget->count(); ++i)
    {
        if(txid==ui->transactionListWidget->item(i)->text())
        {
            return true;
        }
    }
    return false;
}

bool GamePage::addTxidToList(const QString& txid)
{
    if(!isTxidInList(txid))
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(txid);
        ui->transactionListWidget->insertItem(0, newItem);
        return true;
    }
    return false;
}

bool GamePage::removeTxidFromList(const QString& txid)
{
    for(int i = 0; i < ui->transactionListWidget->count(); ++i)
    {
        if(txid==ui->transactionListWidget->item(i)->text())
        {
            delete ui->transactionListWidget->takeItem(i);
            return true;
        }
    }
    return false;
}

std::string GamePage::makeBetPattern()
{
    std::string betTypePattern;
    for(int i=0;i<ui->betListWidget->count();++i)
    {
        if(!betTypePattern.empty())
        {
            betTypePattern+=std::string("+");
        }
        betTypePattern+=ui->betListWidget->item(i)->text().toStdString();
    }
    return betTypePattern;
}

void GamePage::makeBet()
{
    if(walletModel)
    {
        try
        {
            std::shared_ptr<CWallet> wallet = GetWallets()[0];
            if(wallet != nullptr)
            {
                CWallet* const pwallet=wallet.get();
                
                std::vector<double> betAmounts;
                std::vector<std::string> betTypes;
                std::vector<std::vector<int> > betArrays;

                std::string betTypePattern=makeBetPattern();
                int range=ui->rewardRatioSpinBox->value();
                bool isRoulette=false;//lottery
                if(ui->gameTypeComboBox->currentIndex() == 0)//roulette
                {
                    isRoulette=true;
                }
                parseBetType(betTypePattern, range, betAmounts, betTypes, betArrays, isRoulette);

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

                CCoinControl coin_control;
                updateCoinControlState(coin_control);

                std::vector<CRecipient> vecSend;
                createMakeBetDestination(pwallet, sendTo, vecSend);

                LOCK2(cs_main, pwallet->cs_wallet);

                CAmount curBalance = pwallet->GetBalance();

                CReserveKey reservekey(pwallet);
                CAmount nFeeRequired;
                int nChangePosInOut=betArrays.size()+1;
                std::string strFailReason;
                CTransactionRef tx;

                unlockWallet();

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

void GamePage::getBet()
{
    if(walletModel)
    {
        try
        {
            std::shared_ptr<CWallet> wallet = GetWallets()[0];
            if (wallet == nullptr)
            {
                throw std::runtime_error(std::string("No wallet found"));
            }
            CWallet* const pwallet=wallet.get();

            QList<QListWidgetItem*> selectedItems = ui->transactionListWidget->selectedItems();
            if (selectedItems.empty())
            {
                throw std::runtime_error(std::string("No transaction selected"));
            }

            CCoinControl coin_control;
            updateCoinControlState(coin_control);
            int returned_target;
            FeeReason reason;
            CFeeRate feeRate = CFeeRate(walletModel->wallet().getMinimumFee(1000, coin_control, &returned_target, &reason));

            const std::string address = ui->addressLineEdit->text().toStdString();
            double totalAmount = 0;
            UniValue inputs(UniValue::VARR);
            UniValue blockHashes(UniValue::VARR);
            QList<QListWidgetItem *> itemsToRemove;

            for (auto item : selectedItems) {
                currentItem = item;
                QString qtxid = currentItem->text();
                std::string txidIn = qtxid.toStdString();

                UniValue txPrev(UniValue::VOBJ);
                txPrev=findTx(txidIn);
                UniValue prevTxBlockHash(UniValue::VSTR);
                prevTxBlockHash=txPrev["blockhash"].get_str();

                std::tuple<std::string, size_t> betData = getBetData(txPrev);
                std::string betType = std::get<0>(betData);
                const size_t nPrevOut = std::get<1>(betData);

                unsigned int argument=getArgumentFromBetType(betType);
                std::string blockhashStr=txPrev["blockhash"].get_str();
                unsigned int blockhashTmp=blockHashStr2Int(blockhashStr);

                modulo::ModuloOperation moduloOperation;
                moduloOperation.setArgument(argument);
                unsigned int argumentResult = moduloOperation(blockhashTmp);

                for (size_t voutIdx=0;voutIdx<nPrevOut;++voutIdx) {
                    size_t pos=betType.find("+");

                    if (VerifyMakeModuloBetTx().isWinning(betType.substr(0, pos), argument, argumentResult)) {
                        UniValue vout(UniValue::VOBJ);
                        vout=txPrev["vout"][voutIdx];

                        UniValue scriptPubKeyStr(UniValue::VSTR);
                        scriptPubKeyStr=vout["scriptPubKey"]["hex"];

                        const CScript redeemScript = getRedeemScript(pwallet, scriptPubKeyStr.get_str());
                        size_t redeemScriptSize=getRedeemScriptSize(redeemScript);

                        double vout_amount = vout["value"].get_real();
                        int reward=getReward(pwallet, scriptPubKeyStr.get_str());

                        size_t txSize=200+redeemScriptSize;
                        double fee=static_cast<double>(feeRate.GetFee(txSize))/COIN;
                        if(fee>(static_cast<double>(maxTxFee)/COIN))
                        {
                            fee=(static_cast<double>(maxTxFee)/COIN);
                        }
                        else if (fee >= reward * vout_amount)
                        {
                            fee = static_cast<double>(::minRelayTxFee.GetFee(txSize))/COIN;
                        }

                        double amount = (float)reward * vout_amount - fee;
                        totalAmount += amount;

                        UniValue txIn(UniValue::VOBJ);
                        txIn.pushKV("txid", txidIn);
                        txIn.pushKV("vout", static_cast<int>(voutIdx));
                        inputs.push_back(txIn);
                        blockHashes.push_back(prevTxBlockHash);
                    }

                    betType=betType.substr(pos+1);
                }
                itemsToRemove.push_back(currentItem);
            }

            std::string txid;
            if (inputs.size() > 0) {
                std::string totalAmountAsStr = double2str(totalAmount);
                UniValue sendTo(UniValue::VOBJ);
                sendTo.pushKV("address", address);
                sendTo.pushKV("amount", totalAmountAsStr);
                ModuloOperation operation;

                GetBetTxs tx(pwallet, inputs, sendTo, blockHashes, &operation, 0, ui->optInRBF->isChecked());
                unlockWallet();
                tx.signTx();
                std::string hexStr = tx.sendTx().get_str();
                txid+="Winning games txid: " + hexStr;
            }
            else {
                txid += "All games are lost";
            }

            {
                std::lock_guard<std::mutex> lck(mtx);
                for (auto item : itemsToRemove) {
                    delete ui->transactionListWidget->takeItem(ui->transactionListWidget->row(item));
                }
                dumpListToFile(QString("bets.dat"));
            }

            QMessageBox msgBox;
            msgBox.setWindowTitle("Getting bet status:");
            msgBox.setText(QString::fromStdString(txid));
            msgBox.exec();
        }
        catch (std::exception const& e) {
            std::string info=e.what();

            if (info==FAILED_OP_EQUALVERIFY_OPERATION ||
                info==INPUT_NOT_FOUND_OR_SPENT ||
                info==LENGTH_TOO_LARGE ||
                info==OP_RETURN_NOT_FOUND)
            {
                std::lock_guard<std::mutex> lck(mtx);
                delete ui->transactionListWidget->takeItem(ui->transactionListWidget->row(currentItem));
                dumpListToFile(QString("bets.dat"));
            }

            QMessageBox msgBox;
            msgBox.setText(e.what());
            msgBox.exec();
        }
        catch(...) {
            QMessageBox msgBox;
            msgBox.setText("Unknown exception occured");
            msgBox.exec();
        }
    }
}
