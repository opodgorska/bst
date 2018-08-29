// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MODULOTXS_H
#define MODULOTXS_H

#include <games/gamesutils.h>

static const CAmount MAX_PAYOFF=1024*1024*COIN;
static const int32_t MAX_REWARD=1024*1024*1024;

namespace modulo
{

    class ModuloOperation : public ArgumentOperation
    {
    public:
        ModuloOperation(unsigned int argument);
        ModuloOperation();
        void setArgument(unsigned int argument);
        virtual unsigned int operator()(unsigned int blockHash) override;
    };

}

#endif
