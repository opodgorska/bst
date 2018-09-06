// Copyright (c) 2018 Slawek Mozdzonek
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <games/modulo/modulotxs.h>

namespace modulo
{

    ModuloOperation::ModuloOperation(unsigned int argument) : ArgumentOperation(argument) {};

    ModuloOperation::ModuloOperation() {};

    void ModuloOperation::setArgument(unsigned int argument)
    {
        ModuloOperation::setArgument(argument);
    }

    unsigned int ModuloOperation::operator()(unsigned int blockHash)
    {
        blockHash%=argument;
        ++blockHash;

        return blockHash;
    };

}
