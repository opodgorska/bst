// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMETXS_H
#define GAMETXS_H

#include <string>
#include <amount.h>
#include <wallet/wallet.h>

CRecipient createMakeBetDestination(CAmount betSum, const std::string& msg);
unsigned int getArgument(const CScript& redeemScript);

#endif
