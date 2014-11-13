#ifndef RRL_NODE_RELEASER_HH
#define RRL_NODE_RELEASER_HH

#include "rrl_structures.hh"

namespace Rrl {

class NodeReleaser {
    RrlMap& _map;
public:
    NodeReleaser(RrlMap& map) : _map(map) { }

    void releaseNode(RrlMap::iterator node);
};

}

#endif // RRL_NODE_RELEASER_HH
