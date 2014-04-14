#include "rrliptable.hh"
#include <boost/algorithm/string.hpp>

#include <boost/date_time/gregorian/gregorian_io.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/foreach.hpp>
#include "arguments.hh"
#include "recursor_cache.hh"
#include "logger.hh"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

bool operator==(const Netmask& a, const Netmask& b)
{
  return a.match(b.getNetwork());
}

bool operator<(const Netmask& a, const Netmask& b)
{
  return a.compare(&b.getNetwork());
}

RrlIpTable::RrlIpTable() : d_logger(0), d_enabled(false)
{
  try
  {
    RrlSingleLimit defaultLimits;

    d_enabled = ::arg()["rrl-enable"] == "yes";

    if (d_enabled) {

        d_ipv4_prefix_length = boost::lexical_cast<int>(::arg()["rrl-ipv4-prefix-length"]);
        d_ipv6_prefix_length = boost::lexical_cast<int>(::arg()["rrl-ipv6-prefix-length"]);
        d_drop_queries = ::arg()["rrl-drop-requests"] == "yes";
        defaultLimits.netmask = Netmask("0.0.0.0/32");
        defaultLimits.limit_types_number = boost::lexical_cast<int>(::arg()["rrl-limit-types-count"]);
        defaultLimits.limit_ratio_number = boost::lexical_cast<int>(::arg()["rrl-limit-ratio-count"]);
        defaultLimits.ratio = boost::lexical_cast<double>(::arg()["rrl-ratio"]);
        defaultLimits.detection_period = boost::lexical_cast<int>(::arg()["rrl-detection-period"]);
        defaultLimits.blocking_time = boost::lexical_cast<int>(::arg()["rrl-blocking-period"]);

        parseRequestTypes(::arg()["rrl-types"], defaultLimits.types);

        d_limits.push_back(defaultLimits);

        if (::arg()["rrl-enable-log-file"] == "yes") {
          std::string logName = ::arg()["rrl-log-file"];
          d_logger = new Logger("RRL-log");

          if (!d_logger->toFile(logName)) {
            delete d_logger;
            d_logger = 0;
            log() << Logger::Error << "Cannot loggin rrl actions to file. Filename:" << logName;
          }
        }

        if (::arg()["rrl-enable-white-list"] == "yes") {
          parseWhiteList(::arg()["rrl-white-list"]);
        }

        if(::arg()["rrl-enable-special-limits"] == "yes") {
          parseLimitFile(::arg()["rrl-special-limits"]);
        }
    }
  }
  catch (std::exception ex) {
    log() << Logger::Error << "Critical error in RrlIpTable. Exception: " << ex.what();
  }
}

RrlIpTable::Map::iterator RrlIpTable::get(const ComboAddress &addr)
{
  return d_data.find(truncateAddress(addr));
}

Netmask RrlIpTable::truncateAddress(const ComboAddress &addr)
{
  if (addr.sin4.sin_family == AF_INET) {
    return Netmask(addr, d_ipv4_prefix_length);
  }
  else {
    return Netmask(addr, d_ipv6_prefix_length);
  }
}

bool RrlIpTable::decreaseCounters(RrlNode node)
{
  if(!d_enabled)
    return false;

  Map::iterator i = node.iterator;
  if (i->second.last_request_time.is_not_a_date_time())
    return false;

  RrlSingleLimit& lim = node.limit;
  RrlTime now = boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration diff = (now - i->second.last_request_time);
  u_int64_t iDiff = diff.total_milliseconds() / lim.detection_period;
  u_int64_t decRatio = iDiff * lim.limit_ratio_number;
  u_int64_t decTypes = iDiff * lim.limit_types_number;

  Node& n = i->second;
  if (n.counter_ratio > decRatio) {
    n.counter_ratio -= decRatio;
  } else {
    n.counter_ratio = 0;
  }

  if (n.counter_types > decTypes) {
    n.counter_types -= decTypes;
  } else {
      n.counter_types = 0;
  }

  if(!tryBlock(node)) {
    if (n.blocked)
      log() << Logger::Info << i->first.toString() << " released" << std::endl;
    n.blocked = false;
  }

  return n.blocked;
}

RrlNode RrlIpTable::getNode(const ComboAddress& addr)
{
  if(!d_enabled)
    return RrlNode(d_data.end(), true, RrlSingleLimit());

  Netmask address = truncateAddress(addr);
  Map::iterator i = d_data.find(address);

  bool whiteList = d_white_list.count(address);

  if (i == d_data.end()) {
    int length = (addr.sin4.sin_family == AF_INET) ?
          d_ipv4_prefix_length : d_ipv6_prefix_length;
    i = d_data.insert(std::make_pair(Netmask(addr, length), Node())).first;
  }

  return RrlNode(i, whiteList, d_limits[findLimitIndex(addr)]);
}

bool RrlIpTable::tryBlock(RrlNode node)
{
  Map::iterator iter = node.iterator;
  bool res = (iter->second.counter_ratio > node.limit.limit_ratio_number ||
             iter->second.counter_types > node.limit.limit_types_number);

  RrlTime now = boost::posix_time::microsec_clock::local_time();
  if (!node.iterator->second.block_till.is_not_a_date_time()) {
      res = res || (now <= node.iterator->second.block_till);
  }

  if (res && !iter->second.blocked) {
      log() << Logger::Info
            << iter->first.toString() << " was blocked."
            << " Ratio-requests counter: " << iter->second.counter_ratio
            << ", type-requests counter: " << iter->second.counter_types
            << ". Ratio limit: " << node.limit.limit_ratio_number
            << ", types limit: " << node.limit.limit_types_number
            << std::endl;

      RrlTime rtime =  boost::posix_time::microsec_clock::local_time();
      iter->second.block_till = rtime +
          boost::posix_time::milliseconds(node.limit.blocking_time);
  }

  return res;
}

int RrlIpTable::findLimitIndex(const ComboAddress &address)
{
  for (size_t i = 1; i < d_limits.size(); i++) {
    if (d_limits[i].netmask.match(address))
      return i;
  }

  return 0;
}

void RrlIpTable::parseWhiteList(const string &filename)
{
  try
  {
    using boost::property_tree::ptree;
    ptree tree;
    boost::property_tree::info_parser::read_info(filename, tree);

    BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                    tree.get_child("white_list")) {

      d_white_list.insert(node.second.get_value<string>());
    }
    return;
  }
  catch(boost::property_tree::ptree_bad_path err) {
    log() << Logger::Error << err.what();
  }
  catch(boost::property_tree::ptree_bad_data err) {
    log() << Logger::Error << err.what();
  }
  catch(boost::property_tree::info_parser_error err) {
    log() << Logger::Error << err.what();
  }
  log() << Logger::Info << "White list was not set" << std::endl;
  d_white_list.clear();
}

void RrlIpTable::parseLimitFile(const string &filename)
{
  try
  {
    std::vector<RrlSingleLimit> newLimits = d_limits;

    using boost::property_tree::ptree;
    ptree tree;
    boost::property_tree::info_parser::read_info(filename, tree);

    BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                    tree.get_child("nodes")) {

      RrlSingleLimit limit;
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
    return;
  }
  catch(boost::property_tree::ptree_bad_path err) {
    log() << Logger::Error << err.what() << std::endl;
  }
  catch(boost::property_tree::ptree_bad_data err) {
    log() << Logger::Error << err.what() << std::endl;
  }
  catch(boost::property_tree::info_parser_error err) {
    log() << Logger::Error << err.what() << std::endl;
  }
  log() << Logger::Info << "Special limits for netmasks were not set" << std::endl;
}

void RrlIpTable::parseRequestTypes(const string &str, std::set<QType> &types)
{
  std::vector<std::string> splitted;
  boost::split(splitted, str, boost::is_any_of(","));

  BOOST_FOREACH(const std::string& str, splitted) {
    types.insert(QType(QType::chartocode(str.c_str())));
  }
}

bool RrlIpTable::updateRecord(RrlNode rrlNode, QType type)
{
  if(!d_enabled)
    return false;

  if(rrlNode.limit.types.count(type)) {
    Map::iterator i = rrlNode.iterator;

    i->second.last_request_time = boost::posix_time::microsec_clock::local_time();
    i->second.counter_types++;
    i->second.blocked = tryBlock(rrlNode);
    return i->second.blocked;
  }
  return false;
}

bool RrlIpTable::updateRecord(RrlNode rrlNode, double ratio)
{
  if(!d_enabled)
    return false;

  Map::iterator i = rrlNode.iterator;
  if(ratio >= rrlNode.limit.ratio) {
    i->second.counter_ratio++;
    i->second.last_request_time = boost::posix_time::microsec_clock::local_time();

    i->second.blocked = tryBlock(rrlNode);
    return i->second.blocked;
  }
  return false;
}

RrlIpTable& rrlIpTable()
{
  static RrlIpTable table;
  return table;
}
