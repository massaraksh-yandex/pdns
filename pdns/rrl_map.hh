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
    iterator find(const key_type& netmask) { return _map.find(netmask); }
    size_type size() const { return _map.size(); }

    InternalNodePtr addAdderess(const key_type& key);

    void remove(const std::deque<Map::iterator>& nodes);
    void asyncClear();
};

class Stats;
typedef boost::shared_ptr<Stats> StatsPtr;

class Stats {
    static StatsPtr _global;

    AtomicCounter _requests;
    AtomicCounter _lockedNodes;
    Map& _map;

    Stats(Map& map) : _map(map) { }
public:

    void addRequest() { ++_requests; }
    void addLocked() { ++_lockedNodes; }
    void decLocked() { --_lockedNodes; }

    unsigned requests() const { return _requests; }
    unsigned lockedNodes() const { return _lockedNodes; }

    Map::size_type nodes() const { return _map.size(); }

    static StatsPtr setGlobal(Map& map) {
        _global.reset(new Stats(map));
        return _global;
    }

    static StatsPtr global() { return _global; }
};


}

#endif // RRL_MAP_HH
