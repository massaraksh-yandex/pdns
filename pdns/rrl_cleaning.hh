#ifndef RRL_CLEANING_HH
#define RRL_CLEANING_HH

#include "rrl_structures.hh"
#include "rrl_logger.hh"
#include "rrl_map.hh"
#include <deque>

namespace Rrl {

class Cleaning;
typedef boost::shared_ptr<Cleaning> CleaningPtr;


class Cleaning {
protected:
    typedef std::deque<Map::iterator> Queue;
    Map& _map;
    pthread_mutex_t _mutex;

    Cleaning(Map& map) : _map(map) { pthread_mutex_init(&_mutex, 0); }

    void tryUnlockNode(Map::iterator it);

    virtual bool needToDelete(InternalNode& node) const = 0;
    virtual void onClean() = 0;
    virtual void postProcessingQueue(Queue& queue) = 0;

public:
    virtual ~Cleaning() { }

    void clean();
    virtual bool time() const = 0;

    static CleaningPtr make(Map& map);
};

}

#endif // RRL_CLEANING_HH
