#include "rrl_functions.hh"
#include "rrl_logger.hh"
#include "rrl_map.hh"

namespace Rrl {

void releaseNode(InternalNode& node)
{
    Locker mutex(node.mutex);
    Stats::global()->decLocked();
    node.reset();
}

void increaseCounters(RrlNode& node, const PackageInfo& info)
{
    Locker mutex(node.node->mutex);

    node.node->counter_ratio += node.limit.ratio(info._ratio);
    node.node->counter_types += node.limit.types(info._type);
}

bool needBlock(const RrlNode& rrlnode)
{
    const InternalNode& node = *rrlnode.node;
    const SingleLimit& limit = rrlnode.limit;

    bool res = (node.counter_ratio > limit.ratio.limit ||
                node.counter_types > limit.types.limit);

    if (!node.block_till.is_not_a_date_time()) {
        res = res || (now() <= node.block_till);
    }

    return res;
}

void tryBlockNode(RrlNode& node)
{
    InternalNode &rin = *node.node;
    SingleLimit &lim = node.limit;
    if(needBlock(node)) {
        if(!rin.blocked) {
            rin.block(lim.blocking_time);
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

void decreaseCounters(RrlNode &node)
{
#define MINUS(value, numb) \
    if (value > numb) { \
        value -= numb; \
    } else { \
        value = 0; \
    } \

    InternalNode& rin = *node.node;
    if (rin.last_request_time.is_not_a_date_time())
        return;

    SingleLimit& lim = node.limit;
    boost::posix_time::time_duration diff = (now() - rin.last_request_time);
    u_int64_t iDiff = diff.total_milliseconds() / lim.detection_period;
    u_int64_t decRatio = iDiff * lim.ratio.limit;
    u_int64_t decTypes = iDiff * lim.types.limit;

    Locker m(node.node->mutex);
    MINUS(rin.counter_ratio, decRatio);
    MINUS(rin.counter_types, decTypes);

#undef MINUS
}

}
