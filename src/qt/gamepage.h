// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GAMEPAGE_H
#define BITCOIN_QT_GAMEPAGE_H

#include <mutex>
#include <policy/feerate.h>
#include <qt/walletmodel.h>

#include <QWidget>

class WalletModel;
class ClientModel;
class QPlainTextEdit;
class PlatformStyle;
class QButtonGroup;
class QListWidgetItem;

namespace Ui {
    class GamePage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class GamePage : public QWidget
{
    Q_OBJECT

public:
    explicit GamePage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~GamePage();
    
    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

private:
    Ui::GamePage *ui;
    WalletModel *walletModel;
    ClientModel *clientModel;
    std::string changeAddress;
    QListWidgetItem *currentItem;
    bool fFeeMinimized;
    CFeeRate feeRate;
    QButtonGroup *groupFee;
    std::mutex mtx;

private:
    void unlockWallet();
    void dumpListToFile(const QString& fileName);
    void loadListFromFile(const QString& fileName);
    bool isTxidInList(const QString& txid);
    bool addTxidToList(const QString& txid);
    bool removeTxidFromList(const QString& txid);
    void minimizeFeeSection(bool fMinimize);
    void updateFeeMinimizedLabel();
    void updateCoinControlState(CCoinControl& ctrl);
    void clearGameTypeBox();
    std::string makeBetPattern();
    void updateBetNumberLimit();

public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);
    void newTx(const QString &hash);
    void deletedTx(const QString &hash);

private Q_SLOTS:
    void makeBet();
    void getBet();
    
    void updateGameType();
    void updateBetType();
    void addBet();
    void deleteBet();
    
    void updateRewardView();

    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void setMinimumFee();
    void updateFeeSectionControls();
    void updateMinFeeLabel();
    void updateSmartFeeLabel();
    void updateDisplayUnit();
};

#endif // BITCOIN_QT_GAMEPAGE_H
