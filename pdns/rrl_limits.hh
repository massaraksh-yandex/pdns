#ifndef RRL_LIMITS_HH
#define RRL_LIMITS_HH

#include "rrl_structures.hh"
#include <set>

namespace Rrl {

class Limits {
    std::vector<SingleLimit> _data;
    SingleLimit _default;

    void parseRequestTypes(const std::string &str, std::set<QType> &types);
    void initDefault();

public:
    Limits();

    SingleLimit get(const ComboAddress &address) const;

    string reload(const std::string &pathToFile);
};

}

#endif // RRL_LIMITS_HH
