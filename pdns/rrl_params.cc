#include "rrl_params.hh"
#include "arguments.hh"

namespace Rrl {
std::string Params::toString(const std::string& arg)
{
    return ::arg()[arg];
}

bool Params::toBool(const std::string& arg)
{
    return ::arg().mustDo(arg);
}

int Params::toInt(const std::string& arg)
{
    return ::arg().asNum(arg);
}

double Params::toDouble(const std::string& arg)
{
    return ::arg().asDouble(arg);
}

void Params::registerParams()
{
    ::arg().set("rrl-mode", "This parameter tells how the rrl module will action. Possible values: off: rrl module is inactive. truncate: suspicious requests will be truncated. block: suspicious requests will be blocked and there will be no answer. log-only: locking actions will be logged, but all requests will be answered as usual")  = "off";
    ::arg().set("rrl-ipv4-prefix-length", "Significant bits in IPv4 address") = "24";
    ::arg().set("rrl-ipv6-prefix-length", "Significant bits in IPv4 address") = "56";
    ::arg().set("rrl-types", "\'Type Filter\'. Requests with this types are significant") = "ANY,RRSIG";
    ::arg().set("rrl-limit-types-count", "Count of requests with types from rrl-types. When the number of such requests exceed this value, the IP-address will be blocked.") = "50";
    ::arg().set("rrl-ratio", "\'Ratio filter\'. Ratio between answer's size and request's size") = "5.0";
    ::arg().set("rrl-limit-ratio-count", "Count of requests with ratio from rrl-ratio. When the number of such requests exceed this value, the IP-address will be blocked.") = "50";
    ::arg().set("rrl-detection-period", "Time in milliseconds for increasing Filters\' counters. At the end of this period counters are decreased.") = "10";
    ::arg().set("rrl-blocking-period", "If any filter\'s condition are true, the IP-address will be blocked for this time (in ms)") = "20";


    ::arg().set("rrl-logging", "Sets logging type. Can be off, console, file") = "off";
    ::arg().setSwitch("rrl-enable-extra-logging", "Extra logging messages")  = "no";
    ::arg().set("rrl-log-file", "Path for log file") = "";

    ::arg().setSwitch("rrl-enable-white-list", "Enable list of addresses, which cannot be blocked")  = "no";
    ::arg().set("rrl-white-list", "Path to white list") = "";
//    ::arg().setSwitch("rrl-enable-special-limits", "Enable special conditions for some addresses")  = "no";
    ::arg().set("rrl-special-limits", "Path to file with special limits") = "";
    ::arg().set("rrl-cleaning-mode","The way to clean rrl nodes cache. Possible values: off, larger-than, remove-old") = "off";
    ::arg().set("rrl-clean-remove-if-larger","Only if rrl-cleaning-mode == larger-than. If rrl cache size is bigger than this value, the cleaning will been started") = "10000";
    ::arg().set("rrl-clean-remove-n-percent-nodes","Only if rrl-cleaning-mode == larger-than. This value sets percentage of nodes that will be removed") = "10";
    ::arg().set("rrl-clean-remove-every-n-request","Only if rrl-cleaning-mode == remove-old. Every n-th request the cleaning will been started") = "1000";
    ::arg().set("rrl-clean-remove-if-older","Only if rrl-cleaning-mode == remove-old"
                    "If the difference between current second and the last request value from node's ip "
                "address is larger than this value, the node will be erased") = "86400";
}

}
