#include "rrl_structures.hh"
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
}
