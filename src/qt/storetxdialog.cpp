// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <qt/storetxdialog.h>
#include <qt/forms/ui_transactiondescdialog.h>

#include <qt/transactiontablemodel.h>

StoreTxDialog::StoreTxDialog(const QString txid, double fee, int unit, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransactionDescDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Data has been sent"));
    QString strHTML;
    CAmount nTxFee=static_cast<CAmount>(fee*COIN);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    strHTML += "<b>" + tr("Transaction ID") + ":</b> " + txid + "<br>";
    strHTML += "<b>" + tr("Transaction fee") + ":</b> " + BitcoinUnits::formatHtmlWithUnit(unit, -nTxFee) + "<br>";
    strHTML += "</font></html>";
    ui->detailText->setHtml(strHTML);
}

StoreTxDialog::~StoreTxDialog()
{
    delete ui;
}
