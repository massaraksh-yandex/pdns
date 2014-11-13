#include "rrl_cleaning.hh"
#include <queue>

namespace Rrl {

struct SortRrlNodes : std::binary_function<RrlMap::iterator, RrlMap::iterator, bool>
{
    bool operator()(RrlMap::iterator first, RrlMap::iterator second) const
    { return first->second->last_request_time < second->second->last_request_time; }
};

struct EmptyCleaning : public Cleaning {
    EmptyCleaning(RrlMap& map) : Cleaning(map) { }
    void clean() { }
};

struct LargerThan : public Cleaning {
    LargerThan(RrlMap& map) : Cleaning(map) { }

    void clean() {
        std::priority_queue<RrlMap::iterator, std::deque<RrlMap::iterator>, SortRrlNodes> queue;

        for(RrlMap::iterator it = _map.begin(); it != _map.end(); it++) {
            InternalNode& node = *it->second;
            if(node.wasLocked() && node.block_till < now()) {
                releaseNode(node, it->first.toString(), "");
            }

            if(!node.blocked)
                queue.push(it);
        }

        int counter = 0;
        int max = d_cleaning.remove_n_percent_nodes * queue.size();
        while(!queue.empty() && (counter < max)) {
            _map.erase(queue.top());
            queue.pop();
            counter++;
        }
    }
};

}
