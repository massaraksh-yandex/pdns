#ifndef RRL_FUNCTIONS_HH
#define RRL_FUNCTIONS_HH

#include <boost/date_time/posix_time/ptime.hpp>
#include "rrl_structures.hh"
#include "rrl_logger.hh"

namespace Rrl {

void releaseNode(InternalNode &node);
bool needBlock(const InternalNode &node, const SingleLimit& limit);
inline Time now() { return boost::posix_time::microsec_clock::local_time(); }

}

inline bool operator==(const Netmask& a, const Netmask& b) {
    return a.match(b.getNetwork());
}

inline bool operator<(const Netmask& a, const Netmask& b) {
    return a.compare(&b.getNetwork());
}

#endif // RRL_FUNCTIONS_HH
