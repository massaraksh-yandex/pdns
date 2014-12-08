#ifndef RRL_FUNCTIONS_HH
#define RRL_FUNCTIONS_HH

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include "rrl_structures.hh"
#include "rrl_logger.hh"
#include <ctime>

namespace Rrl {

void releaseNode(InternalNode &node);
void tryBlockNode(RrlNode &node, const Time &time);
void updateCounters(RrlNode& node, const PackageInfo& info, const Time &now);

inline Time now() { return boost::posix_time::microsec_clock::local_time(); }
inline bool valid(const Time& time) { return !time.is_not_a_date_time(); }
inline unsigned diff(const Time& a, const Time& b) { return (a - b).total_milliseconds(); }

}

inline bool operator==(const Netmask& a, const Netmask& b) {
    return a.match(b.getNetwork());
}

inline bool operator<(const Netmask& a, const Netmask& b) {
    return a.compare(&b.getNetwork());
}

// Костыли и велосипеды: программируем как умеем
class Locker {
    pthread_mutex_t& _m;
public:
    Locker(pthread_mutex_t& m) : _m(m) { pthread_mutex_lock(&_m); }
    ~Locker() { pthread_mutex_unlock(&_m); }
};

class PoliteLocker {
    pthread_mutex_t& _m;
    bool _locked;
public:
    PoliteLocker(pthread_mutex_t& m) : _m(m), _locked(false) { }
    ~PoliteLocker() { if(_locked) pthread_mutex_unlock(&_m); }
    bool tryLock() { return (_locked = !pthread_mutex_trylock(&_m)); }
    bool locked() const { return _locked; }
};

class TimedLocker {
    pthread_mutex_t& _m;
    timespec _t;
    bool _result;
public:
    TimedLocker(pthread_mutex_t& m) : _m(m), _result(false) { }
    ~TimedLocker() { if(_result)pthread_mutex_unlock(&_m); }

    inline bool tryLock(unsigned microsec) {
        if(!clock_gettime(CLOCK_REALTIME, &_t)) {
            _t.tv_nsec += microsec*1000000;
            return (_result = !pthread_mutex_timedlock(&_m, &_t));
        } else {
            return false;
        }
    }
    inline bool isSucceed() const { return _result; }
};

#endif // RRL_FUNCTIONS_HH
