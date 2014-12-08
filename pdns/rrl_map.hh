#ifndef RRL_MAP_HH
#define RRL_MAP_HH

#include "rrl_structures.hh"
#include "iputils.hh"
#include <map>

namespace Rrl {

struct Map
{
    typedef std::map<Netmask, InternalNodePtr> Cont;
    typedef Cont::iterator iterator;
    typedef Cont::const_iterator const_iterator;
    typedef Cont::value_type value_type;
    typedef Cont::key_type key_type;
    typedef Cont::size_type size_type;

private:
    Cont _map;
    pthread_mutex_t _mutex;

public:
    Map() { pthread_mutex_init(&_mutex, 0); }

    iterator begin() { return _map.begin(); }
    iterator end() { return _map.end(); }

    const_iterator begin() const { return _map.begin(); }
    const_iterator end() const { return _map.end(); }

    iterator find(const key_type& netmask) { return _map.find(netmask); }
    size_type size() const { return _map.size(); }

    InternalNodePtr get(const key_type& key);

    void remove(const std::deque<Map::iterator>& nodes);
    void asyncClear();

    pthread_mutex_t& mutex() { return _mutex; }

    std::string getDBDump();
};

class Stats;
typedef boost::shared_ptr<Stats> StatsPtr;

class Stats {
    static StatsPtr _global;

    AtomicCounter _requests;
    AtomicCounter _lockedNodes;

    // If Map mutex is locked by long operation (remove, getDBDump, etc) we only can get elements
    // So it makes me crying  when all threads are waiting for unlocking.
    // For preventing the TimedLocker is used
    // This variable containing how much times mutex cannot be locked but the executing was continued by timeout
    AtomicCounter _timeoutMutexes;
    Map& _map;

    Stats(Map& map) : _map(map) { }
public:

    void addRequest() { ++_requests; }
    void addLocked() { ++_lockedNodes; }
    void decLocked() { --_lockedNodes; }
    void dropLocked() { _lockedNodes.setToZero(); }
    void addTimeoutMutexes() { ++_timeoutMutexes; }

    unsigned requests() const { return _requests; }
    unsigned lockedNodes() const { return _lockedNodes; }
    unsigned timeoutMutexes() const { return _timeoutMutexes; }

    Map::size_type nodes() const { return _map.size(); }

    static StatsPtr setGlobal(Map& map) {
        _global.reset(new Stats(map));
        return _global;
    }

    static StatsPtr global() { return _global; }
};


}

#endif // RRL_MAP_HH
