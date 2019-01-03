// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>
#include <games/gamesutils.h>
#include <games/gamesverify.h>

bool isBetTx(const CTransaction& tx, int32_t makeBetIndicator)
{
    int32_t txMakeBetVersion=(tx.nVersion ^ makeBetIndicator);
    if(txMakeBetVersion <= CTransaction::MAX_STANDARD_VERSION && txMakeBetVersion >= 1)
    {
        return true;
    }
    return false;
}
