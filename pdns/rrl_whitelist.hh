#ifndef RRL_WHITELIST_HH
#define RRL_WHITELIST_HH

#include "rrl_structures.hh"
#include <set>

namespace Rrl {

class Whitelist
{
    std::set<Netmask> _data;

public:
    Whitelist();

    bool contains(const Netmask &netmask) const;

    string reload(const std::string &pathToFile);
};

}

#endif // RRL_WHITE_LIST_HH
