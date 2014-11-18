#include "rrl_functions.hh"
#include "rrl_logger.hh"

namespace Rrl {

void releaseNode(InternalNode& node)
{
    Locker mutex(node.mutex);
    mutex.lock();
    node.reset();
}

bool needBlock(const InternalNode &node, const SingleLimit& limit)
{
    bool res = (node.counter_ratio > limit.limit_ratio_number ||
                node.counter_types > limit.limit_types_number);

    if (!node.block_till.is_not_a_date_time()) {
        res = res || (now() <= node.block_till);
    }

    return res;
}

}
