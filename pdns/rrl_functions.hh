#ifndef RRL_FUNCTIONS_HH
#define RRL_FUNCTIONS_HH

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include "rrl_structures.hh"
#include "rrl_logger.hh"

namespace Rrl {

void releaseNode(InternalNode &node);
void tryBlockNode(RrlNode &node);
void decreaseCounters(RrlNode& node);
void increaseCounters(RrlNode& node, const PackageInfo& info);

inline Time now() { return boost::posix_time::microsec_clock::local_time(); }

}

inline bool operator==(const Netmask& a, const Netmask& b) {
    return a.match(b.getNetwork());
}

inline bool operator<(const Netmask& a, const Netmask& b) {
    return a.compare(&b.getNetwork());
}

class Locker {
    pthread_mutex_t& _m;
public:

    Locker(pthread_mutex_t& m) : _m(m) { pthread_mutex_lock(&_m); }
    ~Locker() { pthread_mutex_unlock(&_m); }
};

#endif // RRL_FUNCTIONS_HH
