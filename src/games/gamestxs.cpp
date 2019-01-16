// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <games/gamestxs.h>
#include <data/datautils.h>
#include <rpc/server.h>
#include <vector>

CRecipient createMakeBetDestination(CAmount betSum, const std::string& data) {
    std::vector<unsigned char> msg = ParseHexV(data, "Data");

    CRecipient recipient;
    recipient.scriptPubKey << OP_RETURN << msg;
    recipient.nAmount=betSum;
    recipient.fSubtractFeeFromAmount=false;

    return recipient;
}

unsigned int getArgument(const CScript& redeemScript)
{
    CScript::const_iterator it_end=redeemScript.end()-1;
    
    std::vector<unsigned char> argument_(it_end-4, it_end);
    
    unsigned int argument;
    array2type(argument_, argument);
    
    return argument;
}
