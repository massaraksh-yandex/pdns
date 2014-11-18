#include "rrl_impl.hh"
#ifdef WITH_RRL
#include "arguments.hh"
#include "logger.hh"

#include <boost/date_time/gregorian/gregorian_io.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

#include <stdexcept>
#include <queue>
#include <string>

#include "iputils.hh"
#include "qtype.hh"
#include "rrl_logger.hh"
#include "rrl_params.hh"

using std::string;

namespace Rrl {

AddressUtils::AddressUtils() : ipv4_prefix_length(24), ipv6_prefix_length(56)
{
    ipv4_prefix_length = Params::toInt("rrl-ipv4-prefix-length");
    ipv6_prefix_length = Params::toInt("rrl-ipv6-prefix-length");
}

Netmask AddressUtils::truncate(const ComboAddress &addr) {
    if (addr.sin4.sin_family == AF_INET) {
        return Netmask(addr, ipv4_prefix_length);
    }
    else {
        return Netmask(addr, ipv6_prefix_length);
    }
}

Mode Mode::fromString(const string& str) {
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

string Mode::toString(Mode mode) {
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

RrlIpTableImpl::RrlIpTableImpl()
{
    Stats::setGlobal(_map);
    _cleaning = Cleaning::make(_map);
    setMode(Mode::fromString(Params::toString("rrl-mode")));
}

RrlIpTableImpl::RrlIpTableImpl(Mode mode)
{
    Stats::setGlobal(_map);
    _cleaning = Cleaning::make(_map);
    setMode(mode);
}

RrlNode RrlIpTableImpl::getNode(const ComboAddress &addr)
{
    if(!enabled())
        return RrlNode(InternalNodePtr(), addr, true, SingleLimit());

    Stats::global()->addRequest();
    InternalNodePtr rinp;
    Netmask address = _addressUtils.truncate(addr);

    bool whiteList = _whitelist.contains(address);
    Map::iterator i = _map.find(address);

    if (i == _map.end()) {
        rinp = _map.addAdderess(_addressUtils.truncate(addr));
    } else {
        rinp = i->second;
    }

    return RrlNode(rinp, addr, whiteList, _limits.get(addr));
}

string RrlIpTableImpl::reloadWhitelist(const std::string &pathToFile)
{
    if(!enabled()) {
        return "Rrl is disabled";
    }

    string answer = _whitelist.reload(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::string message = "whitelist was reloaded";
        Log::log().message(message);
        return message+"\n";
    }
}

string RrlIpTableImpl::reloadSpecialLimits(const std::string &pathToFile)
{
    if(!enabled()) {
        return "Rrl is disabled\n";
    }

    string answer = _limits.reload(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::string message = "special limits was reloaded";
        Log::log().message(message);
        return message+"\n";
    }
}

std::string RrlIpTableImpl::setMode(Mode mode)
{
    std::ostringstream str;
    str << "mode: " << Mode::toString(mode);
    Log::log().message(str.str());
    _mode = mode;

    if(_mode == Mode::Off) {
        _map.asyncClear();
    }

    return "";
}

string RrlIpTableImpl::information() const
{
    std::ostringstream str;

    str << "Rrl information:" << '\n'
        << "rrl-mode: " << Mode::toString(_mode) << '\n'
        << "total number of nodes: " << Stats::global()->nodes() << '\n'
        << "total number of locked nodes: " << Stats::global()->lockedNodes() << '\n';

    return str.str();
}

}

#endif // WITH_RRL
