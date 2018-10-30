// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>

#include <QMessageBox>
#include <QFileDialog>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/datapage.h>
#include <qt/forms/ui_datapage.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/askpassphrasedialog.h>
#include <qt/storetxdialog.h>

#include <chainparams.h>
#include <wallet/coincontrol.h>
#include <validation.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif
#include <net.h>
#include <utilmoneystr.h>
#include <consensus/validation.h>

#include <data/datautils.h>
#include <data/retrievedatatxs.h>

#include <QSettings>
#include <QButtonGroup>
#include <array>

static constexpr int maxDataSize=MAX_OP_RETURN_RELAY-6;

static const std::array<int, 9> confTargets = { {2, 4, 6, 12, 24, 48, 144, 504, 1008} };
extern int getConfTargetForIndex(int index);
extern int getIndexForConfTarget(int target);

DataPage::DataPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataPage),
    walletModel(0),
    blockSizeDisplay(64),
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
    
    if (!platformStyle->getImagesOnButtons()) {
        ui->fileRetrieveButton->setIcon(QIcon());
        ui->fileStoreButton->setIcon(QIcon());
        ui->retrieveButton->setIcon(QIcon());
        ui->storeButton->setIcon(QIcon());
        ui->fileCheckButton->setIcon(QIcon());
        ui->checkButton->setIcon(QIcon());
    } else {
        ui->fileRetrieveButton->setIcon(platformStyle->SingleColorIcon(":/icons/open"));
        ui->fileStoreButton->setIcon(platformStyle->SingleColorIcon(":/icons/open"));
        ui->retrieveButton->setIcon(platformStyle->SingleColorIcon(":/icons/filesave"));
        ui->storeButton->setIcon(platformStyle->SingleColorIcon(":/icons/send"));
        ui->fileCheckButton->setIcon(platformStyle->SingleColorIcon(":/icons/open"));
        ui->checkButton->setIcon(platformStyle->SingleColorIcon(":/icons/transaction_confirmed"));
    }

    ui->stringRadioButton->setChecked(true);
    connect(ui->retrieveButton, SIGNAL(clicked()), this, SLOT(retrieve()));
    connect(ui->hexRadioButton, SIGNAL(clicked()), this, SLOT(hexRadioClicked()));
    connect(ui->stringRadioButton, SIGNAL(clicked()), this, SLOT(stringRadioClicked()));
    connect(ui->fileRetrieveButton, SIGNAL(clicked()), this, SLOT(fileRetrieveClicked()));
    connect(ui->safeToFileCheckBox, SIGNAL(toggled(bool)), this, SLOT(safeToFileToggled(bool)));
    
    ui->safeToFileCheckBox->setChecked(false);
    ui->fileRetrieveButton->setVisible(false);
    ui->fileRetrieveEdit->setVisible(false);
    ui->fileRetrieveLabel->setVisible(false);

    ui->fileStoreButton->setVisible(false);
    ui->fileStoreEdit->setVisible(false);
    ui->fileStoreLabel->setVisible(false);

    ui->storeMessageRadioButton->setChecked(true);    
    connect(ui->storeMessageRadioButton, SIGNAL(clicked()), this, SLOT(storeMessageRadioClicked()));
    connect(ui->storeFileRadioButton, SIGNAL(clicked()), this, SLOT(storeFileRadioClicked()));
    connect(ui->storeFileHashRadioButton, SIGNAL(clicked()), this, SLOT(storeFileHashRadioClicked()));
    connect(ui->fileStoreButton, SIGNAL(clicked()), this, SLOT(fileStoreClicked()));
    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(store()));
    connect(ui->checkButton, SIGNAL(clicked()), this, SLOT(check()));

    ui->checkMessageRadioButton->setChecked(true);    
    connect(ui->checkMessageRadioButton, SIGNAL(clicked()), this, SLOT(checkMessageRadioClicked()));
    connect(ui->checkFileRadioButton, SIGNAL(clicked()), this, SLOT(checkFileRadioClicked()));
    connect(ui->checkFileHashRadioButton, SIGNAL(clicked()), this, SLOT(checkFileHashRadioClicked()));
    connect(ui->fileCheckButton, SIGNAL(clicked()), this, SLOT(fileCheckClicked()));
    
    ui->checkTextEdit->setVisible(true);
    ui->fileCheckButton->setVisible(false);
    ui->fileCheckLabel->setVisible(false);
    ui->fileCheckEdit->setVisible(false);
}

DataPage::~DataPage()
{
    delete ui;
}

void DataPage::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void DataPage::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void DataPage::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void DataPage::updateFeeMinimizedLabel()
{
    if(!walletModel || !walletModel->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void DataPage::updateMinFeeLabel()
{
    if (walletModel && walletModel->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
            BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), walletModel->wallet().getRequiredFee(1000)) + "/kB")
        );
}

void DataPage::updateCoinControlState(CCoinControl& ctrl)
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

void DataPage::updateSmartFeeLabel()
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

void DataPage::setMinimumFee()
{
    ui->customFee->setValue(walletModel->wallet().getRequiredFee(1000));
}

void DataPage::updateFeeSectionControls()
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

void DataPage::setBalance(const interfaces::WalletBalances& balances)
{
    if(walletModel && walletModel->getOptionsModel())
    {
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), balances.balance));
    }
}

void DataPage::updateDisplayUnit()
{
    setBalance(walletModel->wallet().getBalances());
    ui->customFee->setDisplayUnit(walletModel->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void DataPage::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void DataPage::setModel(WalletModel *model)
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
}

void DataPage::fileRetrieveClicked()
{
    fileToRetrieveName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Files (*.*)"));
    ui->fileRetrieveEdit->setText(fileToRetrieveName);
}

void DataPage::safeToFileToggled(bool checked)
{
    if(checked)
    {
        ui->fileRetrieveButton->setVisible(true);
        ui->fileRetrieveEdit->setVisible(true);
        ui->fileRetrieveLabel->setVisible(true);
    }
    else
    {
        ui->fileRetrieveButton->setVisible(false);
        ui->fileRetrieveEdit->setVisible(false);
        ui->fileRetrieveLabel->setVisible(false);
    }
}

void DataPage::displayInBlocks(QPlainTextEdit* textEdit, const QString& inStr, int blockSize)
{
    textEdit->clear();
    int beg=0;
    while(1)
    {
        QStringRef str=inStr.midRef(beg, blockSize);
        if(str.isNull())
        {
            break;
        }
        textEdit->appendPlainText(str.toString());
        beg+=blockSize;
    }
    QTextCursor textCursor = textEdit->textCursor();
    textCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor,1);
    textEdit->setTextCursor(textCursor);
}

void DataPage::hexRadioClicked()
{
    displayInBlocks(ui->messageRetrieveEdit, hexaValue, blockSizeDisplay);
}

void DataPage::stringRadioClicked()
{
    ui->messageRetrieveEdit->setPlainText(textValue);
}

void DataPage::retrieve()
{
    try
    {
        std::shared_ptr<CWallet> wallet = GetWallets()[0];
        CWallet* pwallet=nullptr;
        if(wallet!=nullptr)
        {
            pwallet=wallet.get();
        }
    
        std::string txid=ui->txidRetrieveEdit->text().toStdString();
        RetrieveDataTxs retrieveDataTxs(txid, pwallet);
        std::vector<char> txData=retrieveDataTxs.getTxData();
        QByteArray dataArray(txData.data(), txData.size());

        textValue = QString::fromUtf8(dataArray);
        hexaValue = dataArray.toHex();
        
        if(ui->hexRadioButton->isChecked())
        {
            displayInBlocks(ui->messageRetrieveEdit, hexaValue, blockSizeDisplay);
        }
        else
        {
            if(QByteArray::fromHex(hexaValue.toLatin1()).contains((char)0x00))
            {
                QMessageBox msgBox;
                msgBox.setText("This message may be truncated. Please use a Hex type view");
                msgBox.exec();
            }
            ui->messageRetrieveEdit->setPlainText(textValue);
        }
        FileWriter fileWriter(ui->fileRetrieveEdit->text());
        fileWriter.write(dataArray);
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


void DataPage::fileStoreClicked()
{
    fileToStoreName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Files (*.*)"));
    ui->fileStoreEdit->setText(fileToStoreName);
}

std::string DataPage::computeHash(QByteArray binaryData)
{
    constexpr size_t hashSize=CSHA256::OUTPUT_SIZE;
    unsigned char fileHash[hashSize];

    CHash256 fileHasher;

    fileHasher.Write(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size());
    fileHasher.Finalize(fileHash);

    return byte2str(&fileHash[0], static_cast<int>(hashSize));                
}

void DataPage::computeHash(QByteArray binaryData, std::vector<unsigned char>& hash)
{
    constexpr size_t hashSize=CSHA256::OUTPUT_SIZE;
    unsigned char fileHash[hashSize];

    CHash256 fileHasher;

    fileHasher.Write(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size());
    fileHasher.Finalize(fileHash);

    hash.insert(hash.end(), &fileHash[0], &fileHash[hashSize]);
}

std::vector<unsigned char> DataPage::getData()
{
    std::vector<unsigned char> retData;
    if(ui->storeMessageRadioButton->isChecked())
    {
        QString qs=ui->messageStoreEdit->toPlainText();
        std::string str=qs.toUtf8().constData();
        
        if(str.length()>maxDataSize)
        {
            throw std::runtime_error(strprintf("Data size is grater than %d bytes", maxDataSize));
        }

        std::vector<unsigned char> retData_(str.begin(), str.end());
        retData.swap(retData_);
    }
    else if(ui->storeFileRadioButton->isChecked() || ui->storeFileHashRadioButton->isChecked())
    {
        QByteArray binaryData;
        FileReader fileReader(ui->fileStoreEdit->text());
        fileReader.read(binaryData);

        if(ui->storeFileRadioButton->isChecked())
        {
            if(binaryData.size()>maxDataSize)
            {
                throw std::runtime_error(strprintf("Data size is grater than %d bytes", maxDataSize));
            }

            std::vector<unsigned char> retData_(binaryData.begin(), binaryData.end());
            retData.swap(retData_);
        }
        else if(ui->storeFileHashRadioButton->isChecked())
        {
            computeHash(binaryData, retData);
        }
    }
    
    return retData;
}

void DataPage::storeMessageRadioClicked()
{
    ui->fileStoreButton->setVisible(false);
    ui->fileStoreEdit->setVisible(false);
    ui->fileStoreLabel->setVisible(false);

    ui->messageStoreEdit->setVisible(true);
}

void DataPage::storeFileRadioClicked()
{
    ui->fileStoreButton->setVisible(true);
    ui->fileStoreEdit->setVisible(true);
    ui->fileStoreLabel->setVisible(true);

    ui->messageStoreEdit->setVisible(false);
}

void DataPage::storeFileHashRadioClicked()
{
    ui->fileStoreButton->setVisible(true);
    ui->fileStoreEdit->setVisible(true);
    ui->fileStoreLabel->setVisible(true);

    ui->messageStoreEdit->setVisible(false);
}

void DataPage::unlockWallet()
{
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void DataPage::store()
{
#ifdef ENABLE_WALLET
    if(walletModel)
    {
        try
        {
            std::shared_ptr<CWallet> wallet = GetWallets()[0];
            if(wallet != nullptr)
            {
                CWallet* const pwallet=wallet.get();
                
                pwallet->BlockUntilSyncedToCurrentChain();

                LOCK2(cs_main, pwallet->cs_wallet);
                
                CAmount curBalance = pwallet->GetBalance();

                std::vector<unsigned char> data = getData();

                CRecipient recipient;
                recipient.scriptPubKey << OP_RETURN << data;
                recipient.nAmount=0;
                recipient.fSubtractFeeFromAmount=false;
                
                std::vector<CRecipient> vecSend;
                vecSend.push_back(recipient);

                CReserveKey reservekey(pwallet);
                CAmount nFeeRequired;
                int nChangePosInOut=1;
                std::string strFailReason;
                CTransactionRef tx;

                unlockWallet();

                CCoinControl coin_control;
                updateCoinControlState(coin_control);

                if(!pwallet->CreateTransaction(vecSend, nullptr, tx, reservekey, nFeeRequired, nChangePosInOut, strFailReason, coin_control))
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

                QString qtxid=QString::fromStdString(tx->GetHash().GetHex());
                
                StoreTxDialog *dlg = new StoreTxDialog(qtxid, static_cast<double>(nFeeRequired)/COIN, walletModel->getOptionsModel()->getDisplayUnit());
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->show();

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
#endif
}

void DataPage::check()
{
    try
    {
        std::string dataHash;
        std::string blockchainHash;
        std::string txid=ui->checkLineEdit->text().toStdString();

        std::shared_ptr<CWallet> wallet = GetWallets()[0];
        CWallet* pwallet=nullptr;
        if(wallet!=nullptr)
        {
            pwallet=wallet.get();
        }
            
        RetrieveDataTxs retrieveDataTxs(txid, pwallet);
        std::vector<char> txData=retrieveDataTxs.getTxData();
        QByteArray dataArray(txData.data(), txData.size());

        if(ui->checkFileRadioButton->isChecked() || ui->checkMessageRadioButton->isChecked())
        {
            blockchainHash = computeHash(dataArray);

            QByteArray binaryData;
            if(ui->checkFileRadioButton->isChecked())
            {
                FileReader fileReader(ui->fileCheckEdit->text());
                fileReader.read(binaryData);
            }
            else if(ui->checkMessageRadioButton->isChecked())
            {
                QString qs=ui->checkTextEdit->toPlainText();
                binaryData = qs.toUtf8();
            }
            dataHash = computeHash(binaryData);
        }
        else
        {
            blockchainHash = dataArray.toHex().toUpper().toStdString();

            QByteArray binaryData;
            FileReader fileReader(ui->fileCheckEdit->text());
            fileReader.read(binaryData);
            dataHash = computeHash(binaryData);
        }

        QMessageBox msgBox;
        if(dataHash.compare(blockchainHash)==0)
        {
            msgBox.setWindowTitle("Check PASS");
            msgBox.setIconPixmap(QPixmap(":/icons/transaction_confirmed"));
        }
        else
        {
            msgBox.setWindowTitle("Check FAIL");
            msgBox.setIconPixmap(QPixmap(":/icons/transaction_conflicted"));
        }
        msgBox.exec();
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

void DataPage::fileCheckClicked()
{
    fileToCheckName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Files (*.*)"));
    ui->fileCheckEdit->setText(fileToCheckName);
}

void DataPage::checkMessageRadioClicked()
{
    ui->checkTextEdit->setVisible(true);
    ui->fileCheckButton->setVisible(false);
    ui->fileCheckLabel->setVisible(false);
    ui->fileCheckEdit->setVisible(false);
}

void DataPage::checkFileRadioClicked()
{
    ui->checkTextEdit->setVisible(false);
    ui->fileCheckButton->setVisible(true);
    ui->fileCheckLabel->setVisible(true);
    ui->fileCheckEdit->setVisible(true);
}

void DataPage::checkFileHashRadioClicked()
{
    ui->checkTextEdit->setVisible(false);
    ui->fileCheckButton->setVisible(true);
    ui->fileCheckLabel->setVisible(true);
    ui->fileCheckEdit->setVisible(true);
}

DataPage::FileWriter::FileWriter(const QString& fileName_) : isOpen(false), file(fileName_) 
{
    if(!fileName_.isEmpty())
    {
        isOpen=file.open(QIODevice::WriteOnly);
        if(!isOpen)
        {
            throw std::runtime_error("Couldn't open the file");
        }
    }
}

DataPage::FileWriter::~FileWriter()
{
    if(isOpen)
    {
        file.close();
    }
}

void DataPage::FileWriter::write(const QByteArray &byteArray)
{
    if(isOpen)
    {
        qint64 bytesWritten=file.write(byteArray);
        if(bytesWritten==-1 || bytesWritten<byteArray.size())
        {
            throw std::runtime_error(std::string("FileWriter.write() failed"));
        }
    }
}

DataPage::FileReader::FileReader(const QString& fileName_) : isOpen(false), file(fileName_) 
{
    if(!fileName_.isEmpty())
    {
        isOpen=file.open(QIODevice::ReadOnly);
        if(!isOpen)
        {
            throw std::runtime_error("Couldn't open the file");
        }
    }
}

DataPage::FileReader::~FileReader()
{
    if(isOpen)
    {
        file.close();
    }
}

void DataPage::FileReader::read(QByteArray &byteArray)
{
    if(isOpen)
    {
        byteArray=file.readAll();
    }
}
