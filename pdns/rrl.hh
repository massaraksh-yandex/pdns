#ifndef RRLIPTABLE_H
#define RRLIPTABLE_H

#include "config.h"
#ifdef WITH_RRL

#include "rrl_structures.hh"
#include <boost/shared_ptr.hpp>
#include <string>

namespace Rrl {
void cleanRrlCache(void*);
class RrlIpTableImpl;
}

class RrlIpTable
{
    friend class RrlNode;
    friend void Rrl::cleanRrlCache(void*);
    boost::shared_ptr<Rrl::RrlIpTableImpl> d_impl;
    pthread_mutex_t d_lock;

public:
    RrlIpTable();

    RrlNode getNode(const ComboAddress& addr);

    bool dropQueries() const;
    bool enabled() const;

    bool timeToClean() const;

    std::string getDBDump() const;
    std::string reloadWhiteList(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end);
    std::string reloadSpecialLimits(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end);
    std::string setRrlMode(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end);
    std::string information() const;
};

inline RrlIpTable& rrlIpTable() {
    static RrlIpTable table;
    return table;
}
#endif // WITH_RRL

#endif // RRLIPTABLE_H
