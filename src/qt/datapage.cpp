// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QMessageBox>
#include <QFileDialog>
#include <qt/datapage.h>
#include <qt/forms/ui_datapage.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/askpassphrasedialog.h>

#include <validation.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif

#include <data/datautils.h>
#include <data/processunspent.h>
#include <data/retrievedatatxs.h>
#include <data/storedatatxs.h>

static constexpr int maxDataSize=MAX_OP_RETURN_RELAY-6;

DataPage::DataPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataPage),
    walletModel(0),
    blockSizeDisplay(64),
    changeAddress("")
{
    ui->setupUi(this);
    
    if (!platformStyle->getImagesOnButtons()) {
        ui->fileRetrieveButton->setIcon(QIcon());
        ui->fileStoreButton->setIcon(QIcon());
        ui->retrieveButton->setIcon(QIcon());
        ui->storeButton->setIcon(QIcon());
    } else {
        ui->fileRetrieveButton->setIcon(platformStyle->SingleColorIcon(":/icons/open"));
        ui->fileStoreButton->setIcon(platformStyle->SingleColorIcon(":/icons/open"));
        ui->retrieveButton->setIcon(platformStyle->SingleColorIcon(":/icons/filesave"));
        ui->storeButton->setIcon(platformStyle->SingleColorIcon(":/icons/send"));
    }

    ui->stringRadioButton->setChecked(true);
    connect(ui->retrieveButton, SIGNAL(clicked()), this, SLOT(retrieve()));
    connect(ui->hexRadioButton, SIGNAL(clicked()), this, SLOT(hexRadioClicked()));
    connect(ui->stringRadioButton, SIGNAL(clicked()), this, SLOT(stringRadioClicked()));
    connect(ui->fileRetrieveButton, SIGNAL(clicked()), this, SLOT(fileRetrieveClicked()));

    ui->fileStoreButton->setEnabled(false);
    ui->fileStoreEdit->setEnabled(false);
    ui->txidStoreEdit->setReadOnly(true);
    ui->storeMessageRadioButton->setChecked(true);    
    connect(ui->storeMessageRadioButton, SIGNAL(clicked()), this, SLOT(storeMessageRadioClicked()));
    connect(ui->storeFileRadioButton, SIGNAL(clicked()), this, SLOT(storeFileRadioClicked()));
    connect(ui->storeFileHashRadioButton, SIGNAL(clicked()), this, SLOT(storeFileHashRadioClicked()));
    connect(ui->fileStoreButton, SIGNAL(clicked()), this, SLOT(fileStoreClicked()));
    connect(ui->storeButton, SIGNAL(clicked()), this, SLOT(store()));
}

DataPage::~DataPage()
{
    delete ui;
}

void DataPage::setModel(WalletModel *model)
{
    walletModel = model;
}

void DataPage::fileRetrieveClicked()
{
    fileToRetrieveName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Files (*.*)"));
    ui->fileRetrieveEdit->setText(fileToRetrieveName);
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
        std::string txid=ui->txidRetrieveEdit->text().toStdString();
        RetrieveDataTxs retrieveDataTxs(txid);
        std::string txData=retrieveDataTxs.getTxData();

        hexaValue = QString::fromStdString(txData);
        textValue = QString::fromLocal8Bit(QByteArray::fromHex(hexaValue.toLatin1()));
        
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

        QByteArray data=QByteArray::fromHex(hexaValue.toUtf8());

        FileWriter fileWriter(ui->fileRetrieveEdit->text());
        fileWriter.write(data);
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

std::string DataPage::getHexStr()
{
    std::string retStr;
    if(ui->storeMessageRadioButton->isChecked())
    {
        QString qs=ui->messageStoreEdit->toPlainText();
        std::string str=qs.toUtf8().constData();
        
        if(str.length()>maxDataSize)
        {
            throw std::runtime_error(strprintf("Data size is grater than %d bytes", maxDataSize));
        }

        retStr = HexStr(str.begin(), str.end());
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

            retStr = byte2str(reinterpret_cast<const unsigned char* >(binaryData.data()), binaryData.size());
        }
        else if(ui->storeFileHashRadioButton->isChecked())
        {
            constexpr size_t hashSize=CSHA256::OUTPUT_SIZE;
            unsigned char fileHash[hashSize];

            CHash256 fileHasher;
            
            fileHasher.Write(reinterpret_cast<unsigned char*>(binaryData.data()), binaryData.size());
            fileHasher.Finalize(fileHash);

            retStr = byte2str(&fileHash[0], static_cast<int>(hashSize));
        }
    }
    
    return retStr;
}

void DataPage::storeMessageRadioClicked()
{
    ui->fileStoreButton->setEnabled(false);
    ui->messageStoreEdit->setEnabled(true);
    ui->fileStoreEdit->setEnabled(false);
}

void DataPage::storeFileRadioClicked()
{
    ui->fileStoreButton->setEnabled(true);
    ui->messageStoreEdit->setEnabled(false);
    ui->fileStoreEdit->setEnabled(true);
}

void DataPage::storeFileHashRadioClicked()
{
    ui->fileStoreButton->setEnabled(true);
    ui->messageStoreEdit->setEnabled(false);
    ui->fileStoreEdit->setEnabled(true);
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
            if(!vpwallets.empty())
            {
                const CWalletRef pwallet=vpwallets[0];

                std::vector<std::string> addresses;
                ProcessUnspent processUnspent(pwallet, addresses);

                std::string hexStr=getHexStr();
                double fee=computeFee(hexStr.length());

                UniValue inputs(UniValue::VARR);
                if(!processUnspent.getUtxForAmount(inputs, fee))
                {
                    throw std::runtime_error(std::string("Insufficient funds"));
                }

                if(changeAddress.empty())
                {
                    changeAddress=getChangeAddress(pwallet);
                }
                
                UniValue sendTo(UniValue::VOBJ);                
                sendTo.pushKV(changeAddress, computeChange(inputs, fee));
                sendTo.pushKV("data", hexStr);

                StoreDataTxs storeDataTxs(pwallet, inputs, sendTo);
                unlockWallet();
                storeDataTxs.signTx();
                std::string txid=storeDataTxs.sendTx().get_str();

                QString qtxid=QString::fromStdString(txid);
                ui->txidStoreEdit->setText(qtxid);
                ui->txidStoreEdit->displayText();
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

DataPage::FileWriter::FileWriter(const QString& fileName_) : isOpen(false), file(fileName_) 
{
    if(!fileName_.isEmpty())
    {
        isOpen=file.open(QIODevice::WriteOnly);
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
