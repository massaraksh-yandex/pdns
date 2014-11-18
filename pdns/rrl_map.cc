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

InternalNodePtr Map::addAdderess(const Map::key_type &key)
{
    Locker m(_mutex);
    return _map.insert(value_type(key, boost::make_shared<InternalNode>())).first->second;
}

void Map::remove(const std::deque<Map::iterator> &nodes)
{
    Locker m(_mutex);
    for(size_type i = 0; i < _map.size(); i++) {
        _map.erase(nodes[i]);
    }
}

void Map::asyncClear() {
    MT->makeThread(asyncClearMap, new MapWithMutex(&_map, &_mutex));
}

}

