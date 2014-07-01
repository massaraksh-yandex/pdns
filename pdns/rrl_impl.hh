#include "config-recursor.h"
#ifndef RRLIPTABLEIMPL_HH
#define RRLIPTABLEIMPL_HH

#ifdef WITH_RRL
#include "rrl_structures.hh"
#include "iputils.hh"
#include "logger.hh"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <set>
#include <map>

struct RrlCleaning {
    enum CleaningMode {
        Off,
        LargerThan,
        RemoveOld
    };

    RrlCleaning()
        : mode(Off), remove_every_n_request(0), remove_if_larger(0),
          remove_if_older(0), remove_n_percent_nodes(0.0)
    { }

    CleaningMode  mode;
    u_int32_t     remove_every_n_request;
    u_int32_t     remove_if_larger;
    u_int32_t     remove_if_older;
    double        remove_n_percent_nodes;
};

namespace Rrl {

typedef std::map<Netmask, boost::shared_ptr<InternalNode> > RrlMap;

class RrlIpTableImpl
{
  Mode   d_mode;
  u_int8_t  d_ipv4_prefix_length;
  u_int8_t  d_ipv6_prefix_length;

  RrlCleaning d_cleaning;

  u_int32_t     d_request_counter;
  u_int32_t     d_locked_nodes;

  std::vector<SingleLimit> d_limits;
  bool              d_limits_enabled;
  std::set<Netmask> d_white_list;
  bool              d_white_list_enabled;
  RrlMap            d_data;
  boost::shared_ptr<Logger>           d_logger;
  bool              d_extra_logging;

  Time now() const { return boost::posix_time::microsec_clock::local_time(); }
  RrlMap::iterator get(const ComboAddress& addr);
  Netmask truncateAddress(const ComboAddress& addr);
  int findLimitIndex(const ComboAddress &address);

  void parseRequestTypes(const string& str, std::set<QType>& types);
  string parseLimitFile(const string& filename);
  string parseWhiteList(const string& filename);

  void initialize(bool readStateFromConfig, Mode mode);
  void initCleaningMode();
  SingleLimit initDefaulLimits();

  void showReleasedMessage(const std::string& address, const std::string& netmask);
  void showReleasedMessage(const std::string& address);

public:
  RrlIpTableImpl();
  RrlIpTableImpl(Mode mode);
  ~RrlIpTableImpl();

  bool timeToClean() const;
  void cleanRrlNodes();
  bool tryBlock(RrlNode node);
  Logger& log();
  Mode mode() const { return d_mode; }
  void setMode(Mode mode);
  RrlNode getNode(const ComboAddress& addr);
  bool decreaseCounters(RrlNode &node);

  bool dropQueries() const { return d_mode.type == Mode::Block; }
  bool enabled() const { return d_mode.type != Mode::Off; }

  string reloadWhiteList();
  string reloadSpecialLimits();
  string information() const;

  static string rrlMessageString;
  static string rrlErrorString;
  static string rrlLockedString;
  static string rrlReleasedString;
  static string rrlReleasedCleaning;
};
}
#endif // WITH_RRL

#endif // RRLIPTABLEIMPL_HH
