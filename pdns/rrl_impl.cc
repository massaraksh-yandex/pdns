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
#include "rrl_params.hh"

using std::string;

namespace Rrl {

//struct SortRrlNodes : std::binary_function<RrlMap::iterator, RrlMap::iterator, bool>
//{
//    bool operator()(RrlMap::iterator first, RrlMap::iterator second) const
//    { return first->second->last_request_time < second->second->last_request_time; }
//};

//string RrlIpTableImpl::rrlMessageString    = "[message]";
//string RrlIpTableImpl::rrlErrorString      = "[error]";
//string RrlIpTableImpl::rrlLockedString     = "[locked]";
//string RrlIpTableImpl::rrlReleasedString   = "[released]";
//string RrlIpTableImpl::rrlReleasedCleaning = "[cleaning]";

//RrlIpTableImpl::RrlIpTableImpl() :
//    d_locked_nodes(0), d_limits_enabled(false), d_white_list_enabled(false),
//    d_extra_logging(false)
//{
//    initialize(true, Mode::Off);
//}

//RrlIpTableImpl::RrlIpTableImpl(Mode mode) :
//    d_locked_nodes(0), d_limits_enabled(false), d_white_list_enabled(false),
//    d_extra_logging(false)
//{
//    initialize(false, mode);
//}

//RrlIpTableImpl::~RrlIpTableImpl()
//{ }

//void RrlIpTableImpl::initialize(bool readStateFromConfig, Mode mode)
//{
//    try {
//        if(readStateFromConfig)
//            setMode(Mode::fromString(::arg()["rrl-mode"]));
//        else
//            setMode(mode);

//        if (enabled()) {
//            d_ipv4_prefix_length = ::arg().asNum("rrl-ipv4-prefix-length");
//            d_ipv6_prefix_length = ::arg().asNum("rrl-ipv6-prefix-length");

//            initCleaningMode();
//            d_limits.push_back(initDefaulLimits());

//            if (::arg().mustDo("rrl-enable-log-file")) {
//                std::string logName = ::arg()["rrl-log-file"];
//                d_logger.reset(new Logger("rrl-log"));

//                if (!d_logger->toFile(logName)) {
//                    d_logger.reset();
//                    log() << Logger::Error << rrlErrorString << " Cannot logging rrl actions to file. Filename: " << logName << std::endl;
//                }
//                d_extra_logging = ::arg().mustDo("rrl-enable-extra-logging");
//            }

//            if (::arg().mustDo("rrl-enable-white-list")) {
//                parseWhiteList(::arg()["rrl-white-list"]);
//            }

//            if(::arg().mustDo("rrl-enable-special-limits")) {
//                parseLimitFile(::arg()["rrl-special-limits"]);
//            }
//            log() << Logger::Info << rrlMessageString << " mode: " << Mode::toString(d_mode) << std::endl;
//            log() << Logger::Info << rrlMessageString << " rrl is initialized" << std::endl;
//        }
//    }
//    catch (std::exception ex) {
//        d_mode = Mode::Off;
//        log() << Logger::Error << rrlErrorString << " Critical error in RrlIpTable. Exception: " << ex.what() << std::endl;
//    }
//}

//void RrlIpTableImpl::initCleaningMode()
//{
//    std::string cleaningMode = ::arg()["rrl-cleaning-mode"];
//    if(!::arg().mustDo("rrl-cleaning-mode")) {
//        d_cleaning.mode = RrlCleaning::Off;
//    } else if(cleaningMode == "larger-than") {
//        d_cleaning.mode = RrlCleaning::LargerThan;
//        d_cleaning.remove_if_larger = ::arg().asNum("rrl-clean-remove-if-larger");
//        d_cleaning.remove_n_percent_nodes = ::arg().asDouble("rrl-clean-remove-n-percent-nodes");
//    } else if(cleaningMode == "remove-old") {
//        d_cleaning.mode = RrlCleaning::RemoveOld;
//        d_cleaning.remove_every_n_request = ::arg().asDouble("rrl-clean-remove-every-n-request");
//        d_cleaning.remove_if_older = ::arg().asDouble("rrl-clean-remove-if-older");
//    } else {
//        d_cleaning.mode = RrlCleaning::Off;
//        log() << Logger::Error << rrlErrorString << "Wrong cleaning mode. value: \'" << cleaningMode << "\'" << std::endl;
//    }
//}

//SingleLimit RrlIpTableImpl::initDefaulLimits()
//{
//    SingleLimit defaultLimits;

//    defaultLimits.netmask = Netmask("0.0.0.0/32");
//    defaultLimits.limit_types_number = ::arg().asNum("rrl-limit-types-count");
//    defaultLimits.limit_ratio_number = ::arg().asNum("rrl-limit-ratio-count");
//    defaultLimits.ratio = ::arg().asDouble("rrl-ratio");
//    defaultLimits.detection_period = ::arg().asNum("rrl-detection-period");
//    defaultLimits.blocking_time = ::arg().asNum("rrl-blocking-period");

//    parseRequestTypes(::arg()["rrl-types"], defaultLimits.types);

//    return defaultLimits;
//}

//Logger &RrlIpTableImpl::log()
//{
//    return d_logger ? *d_logger : theL();
//}

//void RrlIpTableImpl::setMode(Mode mode)
//{
//    d_mode = mode;
//    log() << Logger::Info << rrlMessageString << " mode: " << Mode::toString(d_mode) << std::endl;
//}

//void RrlIpTableImpl::showReleasedMessage(const std::string& address, const std::string& netmask)
//{
//    log() << Logger::Info << rrlReleasedString << " address:" << address << " netmask:" << netmask << std::endl;
//}

//void RrlIpTableImpl::showReleasedMessage(const std::string& address)
//{
//    log() << Logger::Info << rrlReleasedString << rrlReleasedCleaning
//          << " address:" << address << std::endl;
//}

//void RrlIpTableImpl::releaseNode(InternalNode &node, const string& address, const string& netmask)
//{
//    node.blocked = false;
//    node.block_till = Time();
//    d_locked_nodes--;
//    if(netmask.empty())
//        showReleasedMessage(address);
//    else
//        showReleasedMessage(address, netmask);
//}

//RrlMap::iterator RrlIpTableImpl::get(const ComboAddress &addr)
//{
//    return d_data.find(truncateAddress(addr));
//}

//Netmask RrlIpTableImpl::truncateAddress(const ComboAddress &addr)
//{
//    if (addr.sin4.sin_family == AF_INET) {
//        return Netmask(addr, d_ipv4_prefix_length);
//    }
//    else {
//        return Netmask(addr, d_ipv6_prefix_length);
//    }
//}

//bool RrlIpTableImpl::decreaseCounters(RrlNode& node)
//{
//    if(!enabled() || !node.valid())
//        return false;

//    InternalNode& rin = *node.node;
//    if (rin.last_request_time.is_not_a_date_time())
//        return false;

//    SingleLimit& lim = node.limit;
//    boost::posix_time::time_duration diff = (now() - rin.last_request_time);
//    u_int64_t iDiff = diff.total_milliseconds() / lim.detection_period;
//    u_int64_t decRatio = iDiff * lim.limit_ratio_number;
//    u_int64_t decTypes = iDiff * lim.limit_types_number;

//    Mutex m(rin.mutex);
//    if (rin.counter_ratio > decRatio) {
//        rin.counter_ratio -= decRatio;
//    } else {
//        rin.counter_ratio = 0;
//    }

//    if (rin.counter_types > decTypes) {
//        rin.counter_types -= decTypes;
//    } else {
//        rin.counter_types = 0;
//    }

//    if(!tryBlock(node)) {
//        if (rin.blocked) {
//            releaseNode(rin, node.address.toString(), node.limit.netmask.toString());
//        }
//    }

//    return rin.blocked;
//}

//RrlNode RrlIpTableImpl::getNode(const ComboAddress& addr)
//{
//    bool whiteList = true;
//    InternalNodePtr rinp;
//    try
//    {
//        if(!enabled())
//            return RrlNode(InternalNodePtr(), addr, true, SingleLimit());

//        Netmask address = truncateAddress(addr);
//        RrlMap::iterator i = d_data.find(address);
//        d_request_counter++;

//        whiteList = d_white_list.count(address);

//        if (i == d_data.end()) {
//            int length = (addr.sin4.sin_family == AF_INET) ? d_ipv4_prefix_length :
//                                                             d_ipv6_prefix_length;
//            rinp = boost::make_shared<InternalNode>();
//            d_data[Netmask(addr, length)] = rinp;
//        } else {
//            rinp = i->second;
//        }

//    }
//    catch(std::exception& ex) {
//        log() << Logger::Error << rrlErrorString << " " << ex.what() << std::endl;
//    }

//    return RrlNode(rinp, addr, whiteList, d_limits[findLimitIndex(addr)]);

//}

//bool RrlIpTableImpl::tryBlock(RrlNode node)
//{
//    InternalNodePtr iter = node.node;
//    bool res = (iter->counter_ratio > node.limit.limit_ratio_number ||
//                iter->counter_types > node.limit.limit_types_number);

//    if (!node.node->block_till.is_not_a_date_time()) {
//        res = res || (now() <= node.node->block_till);
//    }

//    if (res && !iter->blocked) {
//        iter->blocked = true;
//        d_locked_nodes++;
//        log() << Logger::Info << rrlLockedString << " address:"
//              << node.address.toString() << " netmask:" << node.limit.netmask.toString();
//        if(d_extra_logging) {
//            log() << "; Ratio-requests counter: " << iter->counter_ratio
//                  << "; Type-requests counter: " << iter->counter_types
//                  << "; Ratio limit: " << node.limit.limit_ratio_number
//                  << "; Types limit: " << node.limit.limit_types_number;
//        }
//        log() << std::endl;

//        Time rtime = boost::posix_time::microsec_clock::local_time();
//        iter->block_till = rtime +
//                boost::posix_time::milliseconds(node.limit.blocking_time);
//    }

//    return res;
//}

//int RrlIpTableImpl::findLimitIndex(const ComboAddress &address)
//{
//    for (size_t i = 1; i < d_limits.size(); i++) {
//        if (d_limits[i].netmask.match(address))
//            return i;
//    }

//    return 0;
//}

//string RrlIpTableImpl::parseWhiteList(const string &filename)
//{
//    string error;
//    try
//    {
//        using boost::property_tree::ptree;
//        ptree tree;
//        boost::property_tree::info_parser::read_info(filename, tree);
//        std::set<Netmask> newWhiteList;

//        BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
//                       tree.get_child("white_list")) {

//            newWhiteList.insert(node.second.get_value<string>());
//        }
//        d_white_list.swap(newWhiteList);
//        d_white_list_enabled = true;
//        return "";
//    }
//    catch(boost::property_tree::ptree_bad_path err) {
//        error = err.what();
//    }
//    catch(boost::property_tree::ptree_bad_data err) {
//        error = err.what();
//    }
//    catch(boost::property_tree::info_parser_error err) {
//        error = err.what();
//    }
//    catch(...) {
//        error = "Unknown exception";
//    }
//    d_white_list.clear();
//    d_white_list_enabled = false;

//    error = "White list was not set. Reason: " + error;
//    log() << Logger::Error << rrlErrorString << " " << error << std::endl;
//    return error;
//}

//bool RrlIpTableImpl::timeToClean() const
//{
//    if(!enabled())
//        return false;

//    switch(d_cleaning.mode)
//    {
//    case RrlCleaning::LargerThan:
//        return d_data.size() > d_cleaning.remove_if_larger;
//    case RrlCleaning::RemoveOld:
//        return d_request_counter >= d_cleaning.remove_every_n_request;
//    default:
//        return false;
//    }
//}

//void RrlIpTableImpl::cleanRrlNodes()
//{
//    log() << Logger::Info << rrlMessageString << " cleaning rrl cache" << std::endl;

//    switch(d_cleaning.mode)
//    {
//    case RrlCleaning::Off:return;
//    case RrlCleaning::LargerThan:
//    {
//        std::priority_queue<RrlMap::iterator, std::deque<RrlMap::iterator>, SortRrlNodes> queue;

//        for(RrlMap::iterator it = d_data.begin(); it != d_data.end(); it++) {
//            InternalNode& node = *it->second;
//            if(node.wasLocked() && node.block_till < now()) {
//                releaseNode(node, it->first.toString(), "");
//            }

//            if(!node.blocked)
//                queue.push(it);
//        }

//        int counter = 0;
//        int max = d_cleaning.remove_n_percent_nodes * queue.size();
//        while(!queue.empty() && (counter < max)) {
//            d_data.erase(queue.top());
//            queue.pop();
//            counter++;
//        }
//    };break;
//    case RrlCleaning::RemoveOld:
//    {
//        d_request_counter = 0;
//        Time border = boost::posix_time::microsec_clock::local_time() -
//                boost::posix_time::milliseconds(d_cleaning.remove_if_older);

//        for(RrlMap::iterator it = d_data.begin(); it != d_data.end();) {
//            InternalNode& node = *it->second;

//            if(node.wasLocked() && node.block_till < now()) {
//                releaseNode(node, it->first.toString(), "");
//            }

//            RrlMap::iterator i = it++;
//            if(node.last_request_time < border && !node.blocked)
//                d_data.erase(i);

//        }
//    };break;
//    }
//}

//string RrlIpTableImpl::parseLimitFile(const string &filename)
//{
//    string error;
//    try
//    {
//        std::vector<SingleLimit> newLimits;
//        newLimits.push_back(initDefaulLimits());

//        using boost::property_tree::ptree;
//        ptree tree;
//        boost::property_tree::info_parser::read_info(filename, tree);

//        BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
//                       tree.get_child("nodes")) {

//            SingleLimit limit;
//            const ptree& values = node.second;

//            limit.netmask = Netmask(values.get<string>("address"));
//            limit.limit_types_number = values.get<u_int32_t>("limit-types-count");
//            limit.limit_ratio_number = values.get<u_int32_t>("limit-ratio-count");
//            parseRequestTypes(values.get<string>("types"), limit.types);
//            limit.ratio = values.get<double>("ratio");
//            limit.detection_period = values.get<u_int32_t>("detection-period");
//            limit.blocking_time = values.get<u_int32_t>("blocking-period");

//            newLimits.push_back(limit);
//        }
//        d_limits.swap(newLimits);
//        d_limits_enabled = true;
//        return "";
//    }
//    catch(boost::property_tree::ptree_bad_path err) {
//        error = err.what();
//    }
//    catch(boost::property_tree::ptree_bad_data err) {
//        error = err.what();
//    }
//    catch(boost::property_tree::info_parser_error err) {
//        error = err.what();
//    }
//    catch(...) {
//        error = "unknown error";
//    }
//    error = "Special limits for netmasks were not set. Reason: " + error;
//    d_limits_enabled = false;
//    log() << Logger::Error << rrlErrorString << " " << error << std::endl;
//    return error;
//}

//void RrlIpTableImpl::parseRequestTypes(const string &str, std::set<QType> &types)
//{
//    std::vector<std::string> splitted;
//    boost::split(splitted, str, boost::is_any_of(","));

//    BOOST_FOREACH(const std::string& str, splitted) {
//        types.insert(QType(QType::chartocode(str.c_str())));
//    }
//}

//string RrlIpTableImpl::reloadWhiteList(const std::string& pathToFile)
//{
//    if(!enabled()) {
//        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl white list while rrl is disabled" << std::endl;
//        return "Rrl is disabled";
//    }

//    if(!d_white_list_enabled) {
//        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl white list while white list is disabled by configuration" << std::endl;
//        return "White list is disabled by configuration\n";
//    }

//    string answer = parseWhiteList(pathToFile);

//    if(!answer.empty()) {
//        return answer;
//    } else {
//        std::ostringstream str;
//        str << "Rrl white list was reloaded. New size:" << d_white_list.size() << "\n";
//        return str.str();
//    }
//}

//string RrlIpTableImpl::reloadSpecialLimits(const std::string &pathToFile)
//{
//    if(!enabled()) {
//        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl special limits while rrl is disabled" << std::endl;
//        return "Rrl is disabled\n";
//    }

//    if(!d_limits_enabled) {
//        log() << Logger::Alert << rrlMessageString << "Trying to reload rrl special limits while they are disabled by configuration" << std::endl;;
//        return "Special limits are disabled by configuration\n";
//    }

//    string answer = parseLimitFile(pathToFile);

//    if(!answer.empty()) {
//        return answer;
//    } else {
//        std::ostringstream str;
//        str << "Rrl special limits were reloaded. New size:" << d_limits.size() << "\n";
//        return str.str();
//    }
//}

//string RrlIpTableImpl::information() const
//{
//    std::ostringstream str;

//    str << "Rrl information:" << '\n'
//        << "rrl-mode: " << Mode::toString(d_mode) << '\n'
//        << "total number of nodes: " << d_data.size() << '\n'
//        << "total number of locked nodes: " << d_locked_nodes << '\n'
//        << "size of white list: " << d_white_list.size() << '\n'
//        << "size of limits: " << d_limits.size() << '\n';

//    return str.str();
//}

RrlIpTableImplNew::RrlIpTableImplNew()
    : _log(Log::make()), _limits(_log), _whitelist(_log),
      _stats(_map), _mode(Mode::Off)
{
    _cleaning = Cleaning::make(_map, _log, _stats);
    _mode = Mode::fromString(Params::toString("rrl-mode"));
}

RrlIpTableImplNew::RrlIpTableImplNew(Mode mode)
    : _log(Log::make()), _limits(_log), _whitelist(_log),
      _stats(_map), _mode(mode)
{
    _cleaning = Cleaning::make(_map, _log, _stats);
}

RrlNode RrlIpTableImplNew::getNode(const ComboAddress &addr)
{
    if(!enabled())
        return RrlNode(InternalNodePtr(), addr, true, SingleLimit());

    _stats.addRequest();
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

bool RrlIpTableImplNew::decreaseCounters(RrlNode &node)
{
#define MINUS(value, numb) \
    if (value > numb) { \
        value -= numb; \
    } else { \
        value = 0; \
    } \

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

    {
        Locker m(rin.mutex);
        m.lock();
        MINUS(rin.counter_ratio, decRatio);
        MINUS(rin.counter_types, decTypes);
    }

    if(needBlock(rin, lim)) {
        if(!rin.blocked) {
            rin.block(node.limit.blocking_time);
            _stats.addLocked();
            _log->locked(node);
        }
    }
    else {
        if (rin.blocked) {
            releaseNode(rin);
            _log->released(node.address.toString(), node.limit.netmask.toString());
        }
    }

#undef MINUS
    return rin.blocked;
}

string RrlIpTableImplNew::reloadWhitelist(const std::string &pathToFile)
{
    if(!enabled()) {
        return "Rrl is disabled";
    }

    string answer = _whitelist.reload(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::string message = "whitelist was reloaded";
        _log->message(message);
        return message+"\n";
    }
}

string RrlIpTableImplNew::reloadSpecialLimits(const std::string &pathToFile)
{
    if(!enabled()) {
        return "Rrl is disabled\n";
    }

    string answer = _limits.reload(pathToFile);

    if(!answer.empty()) {
        return answer;
    } else {
        std::string message = "special limits was reloaded";
        _log->message(message);
        return message+"\n";
    }
}

std::string RrlIpTableImplNew::setMode(Mode mode)
{
    std::ostringstream str;
    str << "mode: " << Mode::toString(mode);
    _log->message(str.str());
    _mode = mode;

    if(_mode == Mode::Off) {
        _map.clear();
    }

    return "";
}

string RrlIpTableImplNew::information() const
{
    std::ostringstream str;

    str << "Rrl information:" << '\n'
        << "rrl-mode: " << Mode::toString(_mode) << '\n'
        << "total number of nodes: " << _stats.nodes() << '\n'
        << "total number of locked nodes: " << _stats.lockedNodes() << '\n';

    return str.str();
}

}

#endif // WITH_RRL
