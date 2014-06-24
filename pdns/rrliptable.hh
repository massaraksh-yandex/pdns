#ifndef RRLIPTABLE_H
#define RRLIPTABLE_H

#include <set>
#include <map>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "iputils.hh"
//#include "qtype.hh"
//#include "logger.hh"

struct RrlNode;

class QType;
class Logger;
class Netmask;

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

class RrlIpTable : public boost::noncopyable
{
  typedef boost::posix_time::ptime RrlTime;

  enum CleaningMode
  {
      None,
      LargerThan,
      RemoveOld
  };

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

  struct SortRrlNodes : std::binary_function<Map::iterator, Map::iterator, bool>
  {
      bool operator()(Map::iterator first, Map::iterator second) const
      { return first->second.last_request_time < second->second.last_request_time; }
  };

  u_int8_t  d_ipv4_prefix_length;
  u_int8_t  d_ipv6_prefix_length;
  bool      d_drop_queries;
  bool      d_enabled;

  CleaningMode  d_clean_mode;
  u_int32_t     d_clean_remove_every_n_request;
  u_int32_t     d_clean_remove_if_older;
  u_int32_t     d_clean_remove_if_larger;
  u_int32_t     d_clean_remove_n_nodes;
  u_int32_t     d_request_counter;

  std::vector<RrlSingleLimit> d_limits;
  bool              d_limits_enabled;
  std::set<Netmask> d_white_list;
  bool              d_white_list_enabled;
  Map               d_data;
  Logger*           d_logger;

  Map::iterator get(const ComboAddress& addr);
  Netmask truncateAddress(const ComboAddress& addr);
  bool tryBlock(RrlNode node);
  int findLimitIndex(const ComboAddress &address);

  void parseRequestTypes(const string& str, std::set<QType>& types);
  void cleanRrlNodes();
  string parseLimitFile(const string& filename);
  string parseWhiteList(const string& filename);

  void initCleaningMode();
  RrlSingleLimit initDefaulLimits();

  Logger& log();

public:
  RrlIpTable();
  ~RrlIpTable();

  RrlNode getNode(const ComboAddress& addr, bool tryToClean = true);
  bool decreaseCounters(RrlNode node);

  bool updateRecord(RrlNode rrlNode, QType type);
  bool updateRecord(RrlNode rrlNode, double ratio);

  bool dropQueries() const { return d_drop_queries; }

  string reloadWhiteList();
  string reloadSpecialLimits();
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
