#include "rrl_structures.hh"
#include "rrl_functions.hh"
#include "rrl_params.hh"

namespace Rrl {

AddressUtils::AddressUtils() : ipv4_prefix_length(24), ipv6_prefix_length(56)
{
    ipv4_prefix_length = Params::toInt("rrl-ipv4-prefix-length");
    ipv6_prefix_length = Params::toInt("rrl-ipv6-prefix-length");
}

Netmask AddressUtils::truncate(const ComboAddress &addr)
{
    if (addr.sin4.sin_family == AF_INET) {
        return Netmask(addr, ipv4_prefix_length);
    }
    else {
        return Netmask(addr, ipv6_prefix_length);
    }
}

Mode Mode::fromString(const string& str)
{
    if(str == "off")
        return Mode::Off;
    else if(str == "log-only")
        return Mode::LogOnly;
    else if(str == "truncate")
        return Mode::Truncate;
    else if(str == "block")
        return Mode::Block;
    else
        throw std::invalid_argument(str.c_str());
}

string Mode::toString(Mode mode)
{
    if(mode.type == Mode::Off)
        return "off";
    else if(mode.type == Mode::LogOnly)
        return "log-only";
    else if(mode.type == Mode::Truncate)
        return "truncate";
    else if(mode.type == Mode::Block)
        return "block";
    else {
        std::ostringstream out;
        out << (int)mode.type;
        throw std::invalid_argument(out.str().c_str());
    }
}

void InternalNode::reset() {
    Locker m(mutex);
    m.lock();

    block_till = Time();
    last_request_time = Time();
    counter_ratio = 0;
    counter_types = 0;
    blocked = false;
}

void InternalNode::block(u_int32_t blockinPeriod) {
    Locker m(mutex);
    m.lock();

    blocked = true;
    block_till = now() + boost::posix_time::milliseconds(blockinPeriod);
}

}
