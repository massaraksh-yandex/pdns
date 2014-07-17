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
#include "logger.hh"

using std::string;

bool operator==(const Netmask& a, const Netmask& b)
{
    return a.match(b.getNetwork());
}

bool operator<(const Netmask& a, const Netmask& b)
{
    return a.compare(&b.getNetwork());
}

namespace Rrl {

struct SortRrlNodes : std::binary_function<RrlMap::iterator, RrlMap::iterator, bool>
{
    bool operator()(RrlMap::iterator first, RrlMap::iterator second) const
    { return first->second->last_request_time < second->second->last_request_time; }
};

string RrlIpTableImpl::rrlMessageString    = "[message]";
string RrlIpTableImpl::rrlErrorString      = "[error]";
string RrlIpTableImpl::rrlLockedString     = "[locked]";
string RrlIpTableImpl::rrlReleasedString   = "[released]";
string RrlIpTableImpl::rrlReleasedCleaning = "[cleaning]";


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

RrlIpTableImpl::RrlIpTableImpl() :
    d_locked_nodes(0), d_limits_enabled(false), d_white_list_enabled(false),
    d_extra_logging(false)
{
    initialize(true, Mode::Off);
}

RrlIpTableImpl::RrlIpTableImpl(Mode mode) :
    d_locked_nodes(0), d_limits_enabled(false), d_white_list_enabled(false),
    d_extra_logging(false)
{
    initialize(false, mode);
}

RrlIpTableImpl::~RrlIpTableImpl()
{ }

void RrlIpTableImpl::initialize(bool readStateFromConfig, Mode mode)
{
    try {
        if(readStateFromConfig)
            d_mode = Mode::fromString(::arg()["rrl-mode"]);
        else
            d_mode = mode;

        if (enabled()) {
            d_ipv4_prefix_length = ::arg().asNum("rrl-ipv4-prefix-length");
            d_ipv6_prefix_length = ::arg().asNum("rrl-ipv6-prefix-length");

            initCleaningMode();
            d_limits.push_back(initDefaulLimits());

            if (::arg().mustDo("rrl-enable-log-file")) {
                std::string logName = ::arg()["rrl-log-file"];
                d_logger.reset(new Logger("rrl-log"));

                if (!d_logger->toFile(logName)) {
                    d_logger.reset();
                    log() << Logger::Error << rrlErrorString << " Cannot logging rrl actions to file. Filename: " << logName << std::endl;
                }
                d_extra_logging = ::arg().mustDo("rrl-enable-extra-logging");
            }

            if (::arg().mustDo("rrl-enable-white-list")) {
                parseWhiteList(::arg()["rrl-white-list"]);
            }

            if(::arg().mustDo("rrl-enable-special-limits")) {
                parseLimitFile(::arg()["rrl-special-limits"]);
            }
            log() << Logger::Info << rrlMessageString << " mode: " << Mode::toString(d_mode) << std::endl;
            log() << Logger::Info << rrlMessageString << " rrl is initialized" << std::endl;
        }
    }
    catch (std::exception ex) {
        d_mode = Mode::Off;
        log() << Logger::Error << rrlErrorString << " Critical error in RrlIpTable. Exception: " << ex.what() << std::endl;
    }
}

void RrlIpTableImpl::initCleaningMode()
{
    std::string cleaningMode = ::arg()["rrl-cleaning-mode"];
    if(!::arg().mustDo("rrl-cleaning-mode")) {
        d_cleaning.mode = RrlCleaning::Off;
    } else if(cleaningMode == "larger-than") {
        d_cleaning.mode = RrlCleaning::LargerThan;
        d_cleaning.remove_if_larger = ::arg().asNum("rrl-clean-remove-if-larger");
        d_cleaning.remove_n_percent_nodes = ::arg().asDouble("rrl-clean-remove-n-percent-nodes");
    } else if(cleaningMode == "remove-old") {
        d_cleaning.mode = RrlCleaning::RemoveOld;
        d_cleaning.remove_every_n_request = ::arg().asDouble("rrl-clean-remove-every-n-request");
        d_cleaning.remove_if_older = ::arg().asDouble("rrl-clean-remove-if-older");
    } else {
        d_cleaning.mode = RrlCleaning::Off;
        log() << Logger::Error << rrlErrorString << "Wrong cleaning mode. value: \'" << cleaningMode << "\'" << std::endl;
    }
}

SingleLimit RrlIpTableImpl::initDefaulLimits()
{
    SingleLimit defaultLimits;

    defaultLimits.netmask = Netmask("0.0.0.0/32");
    defaultLimits.limit_types_number = ::arg().asNum("rrl-limit-types-count");
    defaultLimits.limit_ratio_number = ::arg().asNum("rrl-limit-ratio-count");
    defaultLimits.ratio = ::arg().asDouble("rrl-ratio");
    defaultLimits.detection_period = ::arg().asNum("rrl-detection-period");
    defaultLimits.blocking_time = ::arg().asNum("rrl-blocking-period");

    parseRequestTypes(::arg()["rrl-types"], defaultLimits.types);

    return defaultLimits;
}

Logger &RrlIpTableImpl::log()
{
    return d_logger ? *d_logger : theL();
}

void RrlIpTableImpl::setMode(Mode mode)
{
    d_mode = mode;
}

void RrlIpTableImpl::showReleasedMessage(const std::string& address, const std::string& netmask)
{
    log() << Logger::Info << rrlReleasedString << " address:" << address << " netmask:" << netmask << std::endl;
}

void RrlIpTableImpl::showReleasedMessage(const std::string& address)
{
    log() << Logger::Info << rrlReleasedString << rrlReleasedCleaning
          << " address:" << address << std::endl;
}

RrlMap::iterator RrlIpTableImpl::get(const ComboAddress &addr)
{
    return d_data.find(truncateAddress(addr));
}

Netmask RrlIpTableImpl::truncateAddress(const ComboAddress &addr)
{
    if (addr.sin4.sin_family == AF_INET) {
        return Netmask(addr, d_ipv4_prefix_length);
    }
    else {
        return Netmask(addr, d_ipv6_prefix_length);
    }
}

bool RrlIpTableImpl::decreaseCounters(RrlNode& node)
{
    if(!enabled() || !node.valid())
        return false;

    InternalNode& rin = *node.node;
    if (rin.last_request_time.is_not_a_date_time())
        return false;

    SingleLimit& lim = node.limit;
    boost::posix_time::time_duration diff = (now() - rin.last_request_time);
    u_int64_t iDiff = diff.total_milliseconds() / lim.detection_period;
    u_int64_t decRatio = iDiff * lim.limit_ratio_number;
    u_int64_t decTypes = iDiff * lim.limit_types_number;

    if (rin.counter_ratio > decRatio) {
        rin.counter_ratio -= decRatio;
    } else {
        rin.counter_ratio = 0;
    }

    if (rin.counter_types > decTypes) {
        rin.counter_types -= decTypes;
    } else {
        rin.counter_types = 0;
    }

    if(!tryBlock(node)) {
        if (rin.blocked) {
            d_locked_nodes--;
            showReleasedMessage(node.address.toString(), node.limit.netmask.toString());
        }
        rin.blocked = false;
    }

    return rin.blocked;
}

RrlNode RrlIpTableImpl::getNode(const ComboAddress& addr)
{
    bool whiteList = true;
    InternalNodePtr rinp;
    try
    {
        if(!enabled())
            return RrlNode(InternalNodePtr(), addr, true, SingleLimit());

        Netmask address = truncateAddress(addr);
        RrlMap::iterator i = d_data.find(address);
        d_request_counter++;

        whiteList = d_white_list.count(address);

        if (i == d_data.end()) {
            int length = (addr.sin4.sin_family == AF_INET) ? d_ipv4_prefix_length :
                                                             d_ipv6_prefix_length;
            rinp = boost::make_shared<InternalNode>();
            d_data[Netmask(addr, length)] = rinp;
        } else {
            rinp = i->second;
        }

    }
    catch(std::exception& ex) {
        log() << Logger::Error << rrlErrorString << " " << ex.what() << std::endl;
    }

    return RrlNode(rinp, addr, whiteList, d_limits[findLimitIndex(addr)]);

}

bool RrlIpTableImpl::tryBlock(RrlNode node)
{
    InternalNodePtr iter = node.node;
    bool res = (iter->counter_ratio > node.limit.limit_ratio_number ||
                iter->counter_types > node.limit.limit_types_number);

    if (!node.node->block_till.is_not_a_date_time()) {
        res = res || (now() <= node.node->block_till);
    }

    if (res && !iter->blocked) {
        d_locked_nodes++;
        log() << Logger::Info << rrlLockedString << " address:"
              << node.address.toString() << " netmask:" << node.limit.netmask.toString();
        if(d_extra_logging) {
            log() << "; Ratio-requests counter: " << iter->counter_ratio
                  << "; Type-requests counter: " << iter->counter_types
                  << "; Ratio limit: " << node.limit.limit_ratio_number
                  << "; Types limit: " << node.limit.limit_types_number;
        }
        log() << std::endl;

        Time rtime = boost::posix_time::microsec_clock::local_time();
        iter->block_till = rtime +
                boost::posix_time::milliseconds(node.limit.blocking_time);
    }

    return res;
}

int RrlIpTableImpl::findLimitIndex(const ComboAddress &address)
{
    for (size_t i = 1; i < d_limits.size(); i++) {
        if (d_limits[i].netmask.match(address))
            return i;
    }

    return 0;
}

string RrlIpTableImpl::parseWhiteList(const string &filename)
{
    string error;
    try
    {
        using boost::property_tree::ptree;
        ptree tree;
        boost::property_tree::info_parser::read_info(filename, tree);
        std::set<Netmask> newWhiteList;

        BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                       tree.get_child("white_list")) {

            newWhiteList.insert(node.second.get_value<string>());
        }
        d_white_list.swap(newWhiteList);
        d_white_list_enabled = true;
        return "";
    }
    catch(boost::property_tree::ptree_bad_path err) {
        error = err.what();
    }
    catch(boost::property_tree::ptree_bad_data err) {
        error = err.what();
    }
    catch(boost::property_tree::info_parser_error err) {
        error = err.what();
    }
    catch(...) {
        error = "Unknown exception";
    }
    d_white_list.clear();
    d_white_list_enabled = false;

    error = "White list was not set. Reason: " + error;
    log() << Logger::Error << rrlErrorString << " " << error << std::endl;
    return error;
}

bool RrlIpTableImpl::timeToClean() const
{
    if(!enabled())
        return false;

    switch(d_cleaning.mode)
    {
    case RrlCleaning::LargerThan:
        return d_data.size() > d_cleaning.remove_if_larger;
    case RrlCleaning::RemoveOld:
        return d_request_counter >= d_cleaning.remove_every_n_request;
    default:
        return false;
    }
}

void RrlIpTableImpl::cleanRrlNodes()
{
    switch(d_cleaning.mode)
    {
    case RrlCleaning::Off:return;
    case RrlCleaning::LargerThan:
        if(d_data.size() > d_cleaning.remove_if_larger)
        {
            std::priority_queue<RrlMap::iterator, std::deque<RrlMap::iterator>, SortRrlNodes> queue;

            for(RrlMap::iterator it = d_data.begin(); it != d_data.end(); it++) {
                InternalNode& node = *it->second;
                if(node.wasLocked() && node.block_till < now()) {
                    node.blocked = false;
                    showReleasedMessage(it->first.toString());
                }

                if(!node.blocked)
                    queue.push(it);
            }

            int counter = 0;
            int max = d_cleaning.remove_n_percent_nodes * queue.size();
            while(!queue.empty() && (counter < max)) {
                d_data.erase(queue.top());
                queue.pop();
                counter++;
            }
        }
        ;break;
    case RrlCleaning::RemoveOld:
        if(d_request_counter >= d_cleaning.remove_every_n_request)
        {
            d_request_counter = 0;
            for(RrlMap::iterator it = d_data.begin(); it != d_data.end();) {
                Time border = boost::posix_time::microsec_clock::local_time() -
                        boost::posix_time::milliseconds(d_cleaning.remove_if_older);
                InternalNode& node = *it->second;

                if(node.wasLocked() && node.block_till < now()) {
                    node.blocked = false;
                    showReleasedMessage(it->first.toString());
                }

                if(node.last_request_time < border && !node.blocked)
                    d_data.erase(it++);
            }
        }
        ;break;
    }
}

string RrlIpTableImpl::parseLimitFile(const string &filename)
{
    string error;
    try
    {
        std::vector<SingleLimit> newLimits;
        newLimits.push_back(initDefaulLimits());

        using boost::property_tree::ptree;
        ptree tree;
        boost::property_tree::info_parser::read_info(filename, tree);

        BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                       tree.get_child("nodes")) {

            SingleLimit limit;
            const ptree& values = node.second;

            limit.netmask = Netmask(values.get<string>("address"));
            limit.limit_types_number = values.get<u_int32_t>("limit-types-count");
            limit.limit_ratio_number = values.get<u_int32_t>("limit-ratio-count");
            parseRequestTypes(values.get<string>("types"), limit.types);
            limit.ratio = values.get<double>("ratio");
            limit.detection_period = values.get<u_int32_t>("detection-period");
            limit.blocking_time = values.get<u_int32_t>("blocking-period");

            newLimits.push_back(limit);
        }
        d_limits.swap(newLimits);
        d_limits_enabled = true;
        return "";
    }
    catch(boost::property_tree::ptree_bad_path err) {
        error = err.what();
    }
    catch(boost::property_tree::ptree_bad_data err) {
        error = err.what();
    }
    catch(boost::property_tree::info_parser_error err) {
        error = err.what();
    }
    catch(...) {
        error = "unknown error";
    }
    error = "Special limits for netmasks were not set. Reason: " + error;
    d_limits_enabled = false;
    log() << Logger::Error << rrlErrorString << " " << error << std::endl;
    return error;
}

void RrlIpTableImpl::parseRequestTypes(const string &str, std::set<QType> &types)
{
    std::vector<std::string> splitted;
    boost::split(splitted, str, boost::is_any_of(","));

    BOOST_FOREACH(const std::string& str, splitted) {
        types.insert(QType(QType::chartocode(str.c_str())));
    }
}

string RrlIpTableImpl::reloadWhiteList(const std::string& pathToFile)
{
    if(!enabled()) {
        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl white list while rrl is disabled" << std::endl;
        return "Rrl is disabled";
    }

    if(!d_white_list_enabled) {
        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl white list while white list is disabled by configuration" << std::endl;
        return "White list is disabled by configuration\n";
    }

    string answer = parseWhiteList(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::ostringstream str;
        str << "Rrl white list was reloaded. New size:" << d_white_list.size() << "\n";
        return str.str();
    }
}

string RrlIpTableImpl::reloadSpecialLimits(const std::string &pathToFile)
{
    if(!enabled()) {
        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl special limits while rrl is disabled" << std::endl;
        return "Rrl is disabled\n";
    }

    if(!d_limits_enabled) {
        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl special limits while they are disabled by configuration" << std::endl;;
        return "Special limits are disabled by configuration\n";
    }

    string answer = parseLimitFile(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::ostringstream str;
        str << "Rrl special limits were reloaded. New size:" << d_limits.size() << "\n";
        return str.str();
    }
}

string RrlIpTableImpl::information() const
{
    std::ostringstream str;

    str << "Rrl information:" << '\n'
        << "rrl-mode: " << Mode::toString(d_mode) << '\n'
        << "total number of nodes: " << d_data.size() << '\n'
        << "total number of locked nodes: " << d_locked_nodes << '\n'
        << "size of white list: " << d_white_list.size() << '\n'
        << "size of limits: " << d_limits.size() << '\n';

    return str.str();
}

}

#endif // WITH_RRL
