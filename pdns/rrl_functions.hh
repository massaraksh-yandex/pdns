#ifndef RRL_FUNCTIONS_HH
#define RRL_FUNCTIONS_HH

#include "rrl_structures.hh"
#include "rrl_logger.hh"

namespace Rrl {

void releaseNode(InternalNode &node);
bool needBlock(const InternalNode &node, const SingleLimit& limit);

}

#endif // RRL_FUNCTIONS_HH
