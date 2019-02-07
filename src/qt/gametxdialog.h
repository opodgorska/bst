// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GAMETXDIALOG_H
#define BITCOIN_QT_GAMETXDIALOG_H

#include <QDialog>

namespace Ui {
    class TransactionDescDialog;
}


/** Dialog showing transaction details. */
class GameTxDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GameTxDialog(const QString& txid, const QString& game, const QString& ratio, const QString& bet, double fee, int unit, QWidget *parent = 0);
    ~GameTxDialog();

private:
    Ui::TransactionDescDialog *ui;
};

#endif // BITCOIN_QT_GAMETXDIALOG_H
