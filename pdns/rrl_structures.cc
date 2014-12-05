#include "rrl_structures.hh"
#include "rrl_functions.hh"
#include "rrl_params.hh"
#include "rrl.hh"
#include "rrl_impl.hh"

namespace Rrl {

void InternalNode::reset() {
    Locker m(mutex);

    at_least_block_till = Time();
    last_request_time = Time();
    counter_ratio = 0;
    counter_types = 0;
    blocked = false;
}

void InternalNode::block(u_int32_t blockinPeriod, const Time& now) {
    Locker m(mutex);

    blocked = true;
    at_least_block_till = now + boost::posix_time::milliseconds(blockinPeriod);
}

bool InternalNode::wasLocked() const
{
     return Rrl::valid(at_least_block_till);
}

}

bool RrlNode::checkState() const {
    return rrlIpTable().d_impl->mode() != Rrl::Mode::LogOnly;
}

void RrlNode::update(const PackageInfo &info) {
    RrlIpTable& table = rrlIpTable();
    if(!table.enabled() || !valid())
        return;

    Rrl::Time time = Rrl::now();

    if(!isInWhiteList) {
        Rrl::updateCounters(*this, info, time);
        Rrl::tryBlockNode(*this, time);
    }

    Locker mutex(node->mutex);
    node->last_request_time = time;
}

bool RrlNode::blocked() const {
    if(!rrlIpTable().enabled() || isInWhiteList)
        return false;

    bool a = checkState();

    return node->blocked && a;
}

