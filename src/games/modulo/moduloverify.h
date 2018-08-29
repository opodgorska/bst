// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MODULOVERIFY_H
#define MODULOVERIFY_H

#include <games/gamesverify.h>

namespace modulo
{

    class GetModuloReward : public GetReward
    {
    public:
        virtual int operator()(const std::string& betType, unsigned int modulo) override;
    };

    bool txVerify(const CTransaction& tx, CAmount in, CAmount out, CAmount& fee);

}

#endif
