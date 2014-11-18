#include "rrl_functions.hh"
#include "rrl_cleaning.hh"
#include "rrl_params.hh"
#include <algorithm>
#include <queue>
#include <deque>

namespace Rrl {

struct EmptyCleaning : public Cleaning {
    EmptyCleaning(Map& map) : Cleaning(map) { }

    bool time() const { return false; }
    bool needToDelete(InternalNode&) const { return false; }

    void onClean() { }
    void postProcessingQueue(Queue&) { }
};

struct LargerThan : public Cleaning {
    struct SortRrlNodes : std::binary_function<Map::iterator, Map::iterator, bool>
    {
        bool operator()(Map::iterator first, Map::iterator second) const
        { return first->second->last_request_time < second->second->last_request_time; }
    };

    u_int32_t remove_n_percent_nodes;
    u_int32_t remove_if_larger;

    LargerThan(Map& map)
        : Cleaning(map), remove_n_percent_nodes(0),
          remove_if_larger(0) {
        remove_n_percent_nodes = Params::toInt("rrl-clean-remove-n-percent-nodes");
        remove_if_larger = Params::toInt("rrl-clean-remove-if-larger");
    }

    bool time() const { return _map.size() > remove_if_larger; }

    bool needToDelete(InternalNode& node) const { return !node.blocked; }

    void onClean() { }

    void postProcessingQueue(Queue& queue) {
        std::make_heap(queue.begin(), queue.end(), SortRrlNodes());
        std::sort_heap(queue.begin(), queue.end(), SortRrlNodes());
        int max = remove_n_percent_nodes * 0.01 * queue.size();
        Queue newQueue(queue.begin(), queue.begin() + max);
        queue.swap(newQueue);
    }
};

struct RemoveOld : public Cleaning {
    u_int32_t remove_every_n_request;
    u_int32_t remove_if_older;
    u_int32_t lastRequestsCounter;
    Time border;

    RemoveOld(Map& map)
        : Cleaning(map), remove_every_n_request(0),
          remove_if_older(0), lastRequestsCounter(0) {
        remove_every_n_request = Params::toInt("rrl-clean-remove-every-n-request");
        remove_if_older = Params::toInt("rrl-clean-remove-if-older");
    }

    bool needToDelete(InternalNode& node) const { return node.last_request_time < border && !node.blocked; }

    void postProcessingQueue(Queue&) { }

    void onClean() {
        border = boost::posix_time::microsec_clock::local_time() -
                 boost::posix_time::milliseconds(remove_if_older);
        lastRequestsCounter = Stats::global()->requests();
    }

    bool time() const { return Stats::global()->requests() - lastRequestsCounter > remove_every_n_request; }
};

void Cleaning::tryUnlockNode(Map::iterator it)
{
    InternalNode& node = *it->second;

    if(node.wasLocked() && node.block_till < now()) {
        releaseNode(node);
        Log::log().cleaning(it->first.toString());
    }
}

void Cleaning::clean()
{
    Queue queue;
    Log::log().message("cleaning rrl cache");
    onClean();
    for(Map::iterator it = _map.begin(); it != _map.end(); it++) {
        tryUnlockNode(it);
        InternalNode& node = *it->second;

        if(needToDelete(node)) {
            queue.push_back(it);
        }
    }
    postProcessingQueue(queue);
    _map.remove(queue);
}

CleaningPtr Cleaning::make(Map& map)
{
    std::string type = Params::toString("rrl-cleaning-mode");

    if(type == "off") {
        return CleaningPtr(new EmptyCleaning(map));
    } else if(type == "larger-than") {
        return CleaningPtr(new LargerThan(map));
    } else if(type == "remove-old") {
        return CleaningPtr(new RemoveOld(map));
    } else {
        std::ostringstream str;
        str << "wrong cleaning mode == " << type << ". Rrl cleaning is off";
        Log::log().error(str.str());
        return CleaningPtr(new EmptyCleaning(map));
    }
}

}
