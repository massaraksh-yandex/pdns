#include "rrl_map.hh"
#include "rrl_functions.hh"
#include "syncres.hh"

namespace Rrl {

StatsPtr Stats::_global;

struct MapWithMutex {
    Map::Cont* map;
    pthread_mutex_t* mutex;
    MapWithMutex(Map::Cont* m, pthread_mutex_t* mu) : map(m), mutex(mu) { }
};

void asyncClearMap(void* d) {
    MapWithMutex* data = (MapWithMutex*)d;

    Locker m(*data->mutex);
    data->map->clear();

    delete data;
}

InternalNodePtr Map::get(const key_type &key)
{
    iterator it = _map.find(key);

    TimedLocker locker(_mutex);
    if(it == end()) {
        if(locker.tryLock(100)) {
            return _map.insert(value_type(key, boost::make_shared<InternalNode>())).first->second;
        } else {
            Stats::global()->addTimeoutMutexes();
            return boost::make_shared<InternalNode>();
        }
    } else {
        return it->second;
    }
}

void Map::remove(const std::deque<Map::iterator> &nodes)
{
//    Locker m(_mutex);
    for(size_type i = 0; i < nodes.size(); i++) {
        _map.erase(nodes[i]);
    }
}

void Map::asyncClear() {
    MT->makeThread(asyncClearMap, new MapWithMutex(&_map, &_mutex));
}

std::string Map::getDBDump()
{
    std::ostringstream res;

    Locker lock(_mutex);

    for(const_iterator it = begin(); it != end(); it++) {
        const InternalNode& node = *it->second;
        res << it->first.toString() << ": " << node.toString() << "\n";
    }

    return res.str();
}

}

