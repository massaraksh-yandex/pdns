#ifndef RRLIPTABLE_H
#define RRLIPTABLE_H

#include <list>
#include <map>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "iputils.hh"
#include "qtype.hh"
#include "logger.hh"

struct RrlNode;

struct RrlSingleLimit
{
  Netmask netmask;

  u_int32_t limit_ratio_number;
  u_int32_t limit_types_number;

  std::set<QType> types;
  double ratio;

  u_int32_t detection_period;
  u_int32_t blocking_time;
};

class RrlIpTable
{
  typedef boost::posix_time::ptime RrlTime;

  struct Node
  {
     RrlTime block_till;
     RrlTime last_request_time;
     u_int64_t counter_ratio;
     u_int64_t counter_types;
     bool blocked;

     Node() : block_till(), last_request_time(), counter_ratio(0),
       counter_types(0), blocked(false)
     { }
  };

public:
  typedef std::map<Netmask, Node> Map;

private:

  u_int8_t  d_ipv4_prefix_length;
  u_int8_t  d_ipv6_prefix_length;
  bool      d_drop_queries;
  bool      d_enabled;

  std::vector<RrlSingleLimit> d_limits;
  std::set<Netmask> d_white_list;
  Map               d_data;
  Logger*           d_logger;

  Map::iterator get(const ComboAddress& addr);
  Netmask truncateAddress(const ComboAddress& addr);
  bool tryBlock(RrlNode node);
  int findLimitIndex(const ComboAddress &address);

  void parseRequestTypes(const string& str, std::set<QType>& types);
  void parseLimitFile(const string& filename);
  void parseWhiteList(const string& filename);

  Logger& log() { return d_logger ? *d_logger : theL(); }

public:
  RrlIpTable();
  ~RrlIpTable() { delete d_logger; }

  RrlNode getNode(const ComboAddress& addr);
  bool decreaseCounters(RrlNode node);

  bool updateRecord(RrlNode rrlNode, QType type);
  bool updateRecord(RrlNode rrlNode, double ratio);

  bool dropQueries() const { return d_drop_queries; }
};

struct RrlNode
{
  bool isInWhiteList;
  RrlIpTable::Map::iterator iterator;
  RrlSingleLimit limit;

  RrlNode(RrlIpTable::Map::iterator it, bool inWhiteList, const RrlSingleLimit& lim)
    : isInWhiteList(inWhiteList), iterator(it), limit(lim)
  { }

  bool blocked() const { return iterator->second.blocked; }
};

RrlIpTable& rrlIpTable();

#endif // RRLIPTABLE_H
