#ifndef RRL_CLEANING_HH
#define RRL_CLEANING_HH

#include "rrl_structures.hh"

namespace Rrl {

class Cleaning {
    RrlMap& _map;
public:
    Cleaning(RrlMap& map) : _map(map) { }
    virtual void clean() = 0;
};

}

#endif // RRL_CLEANING_HH
