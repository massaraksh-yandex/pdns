#ifndef RRL_MAP_HH
#define RRL_MAP_HH

#include "rrl_structures.hh"
#include "iputils.hh"
#include <map>

namespace Rrl {

struct Map
{
    typedef std::map<Netmask, InternalNodePtr > cont;
    typedef cont::iterator iterator;
    typedef cont::value_type value_type;
    typedef cont::key_type key_type;
    typedef cont::size_type size_type;

private:
    cont _map;
    pthread_mutex_t _mutex;

public:
    Map() { pthread_mutex_init(&_mutex, 0); }

    iterator begin() { return _map.begin(); }
    iterator end() { return _map.end(); }
    iterator find(const key_type& netmask) { return _map.find(netmask); }
    size_type size() const { return _map.size(); }

    template<class Container>
    void remove(const Container& nodes) {
        Locker m(_mutex);
        m.lock();
        for(typename Container::size_type i = 0; i < _map.size(); i++) {
            _map.erase(nodes[i]);
        }
    }

    InternalNodePtr addAdderess(const key_type& key) {
        Locker m(_mutex);
        m.lock();
        return _map.insert(value_type(key, boost::make_shared<InternalNode>())).first->second;
    }

    void clear() {
        Locker m(_mutex);
        m.lock();
        _map.clear();
    }
};


class Stats;
typedef boost::shared_ptr<Stats> StatsPtr;

class Stats {
protected:
    AtomicCounter _requests;
    AtomicCounter _lockedNodes;
    Map& _map;

public:
    Stats(Map& map) : _map(map) { }

    void addRequest() { ++_requests; }
    void addLocked() { ++_lockedNodes; }

    unsigned requests() const { return _requests; }
    unsigned lockedNodes() const { return _lockedNodes; }

    Map::size_type nodes() const { return _map.size(); }
};

}

#endif // RRL_MAP_HH
