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

#include "rrl_map.hh"
#include "rrl_cleaning.hh"
#include "rrl_limits.hh"
#include "rrl_logger.hh"
#include "rrl_whitelist.hh"
#include "rrl_functions.hh"

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

class RrlIpTableImplNew {
    LogPtr _log;

    Map _map;
    Limits _limits;
    Whitelist _whitelist;
    Stats _stats;
    AddressUtils _addressUtils;
    Mode _mode;

    CleaningPtr _cleaning;

public:
    RrlIpTableImplNew();
    RrlIpTableImplNew(Mode mode);

    RrlNode getNode(const ComboAddress& addr);

    bool dropQueries() const { return _mode.type == Mode::Block; }
    bool enabled() const { return _mode.type != Mode::Off; }

    bool timeToClean() const {
        return _cleaning->time();
    }

    bool decreaseCounters(RrlNode &node);

    LogPtr log() const { return _log; }

    string reloadWhitelist(const std::string &pathToFile);
    string reloadSpecialLimits(const std::string &pathToFile);
    string setRrlMode(Mode mode);
    string information() const;
};

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

  void releaseNode(InternalNode &node, const string &address, const string &netmask);

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

  string reloadWhiteList(const std::string &pathToFile);
  string reloadSpecialLimits(const std::string &pathToFile);
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
