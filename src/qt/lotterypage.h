// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_LOTTERYPAGE_H
#define BITCOIN_QT_LOTTERYPAGE_H

#include <qt/walletmodel.h>

#include <QFile>
#include <QWidget>
#include <univalue.h>

class WalletModel;
class QPlainTextEdit;
class PlatformStyle;
class QButtonGroup;

namespace Ui {
    class LotteryPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class LotteryPage : public QWidget
{
    Q_OBJECT

public:
    explicit LotteryPage(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~LotteryPage();
    void setModel(WalletModel *model);

private:
    Ui::LotteryPage *ui;
    WalletModel *walletModel;
    std::string changeAddress;

private:
    void unlockWallet();

private Q_SLOTS:
    void makeBet();
    void getBet();
};

#endif // BITCOIN_QT_LOTTERYPAGE_H
