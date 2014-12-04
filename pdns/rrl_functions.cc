#include "rrl_functions.hh"
#include "rrl_logger.hh"
#include "rrl_map.hh"

namespace Rrl {

void releaseNode(InternalNode& node)
{
    Stats::global()->decLocked();
    node.reset();
}

bool needBlock(const RrlNode& rrlnode, const Time& now)
{
    const InternalNode& node = *rrlnode.node;
    const SingleLimit& limit = rrlnode.limit;

    bool res = (node.counter_ratio > limit.ratio.limit ||
                node.counter_types > limit.types.limit);

    if (valid(node.at_least_block_till)) {
        res = res || (now <= node.at_least_block_till);
    }

    return res;
}

void tryBlockNode(RrlNode& node, const Time& time)
{
    InternalNode &rin = *node.node;
    SingleLimit &lim = node.limit;
    if(needBlock(node, time)) {
        if(!rin.blocked) {
            rin.block(lim.blocking_time, time);
            Stats::global()->addLocked();
            Log::log().locked(node);
        }
    }
    else {
        if (rin.blocked) {
            releaseNode(rin);
            Log::log().released(node.address.toString(), node.limit.netmask.toString());
        }
    }
}

inline void uMinus(u_int64_t& value, u_int64_t numb) {
    if (value > numb) {
        value -= numb;
    } else {
        value = 0;
    }
}

void updateCounters(RrlNode &node, const PackageInfo& info, const Time& now)
{
    InternalNode& rin = *node.node;

    int ratioInc = node.limit.ratio(info._ratio);
    int typesInc = node.limit.types(info._type);

    Locker m(node.node->mutex);

    rin.counter_ratio += ratioInc;
    rin.counter_types += typesInc;

    if (!valid(rin.last_request_time))
        return;

    SingleLimit& lim = node.limit;
    boost::posix_time::time_duration diff = (now - rin.last_request_time);
    double iDiff = diff.total_milliseconds() / (double)lim.detection_period;
    u_int64_t decRatio = iDiff * lim.ratio.limit;
    u_int64_t decTypes = iDiff * lim.types.limit;

    uMinus(rin.counter_ratio, decRatio);
    uMinus(rin.counter_types, decTypes);
}

}
