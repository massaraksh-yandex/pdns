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

class RrlIpTableImpl;


struct RrlImplContaining
{
    RrlIpTableImpl& impl;

    RrlImplContaining(RrlIpTableImpl* im) : impl(*im) { }
    virtual ~RrlImplContaining() {}
};

class Messages : public RrlImplContaining {
    static string rrlMessageString;
    static string rrlErrorString;
    static string rrlLockedString;
    static string rrlReleasedString;
    static string rrlReleasedCleaning;

    boost::shared_ptr<Logger> d_logger;
    bool d_extra_logging;

public:
    Messages(RrlIpTableImpl* im) : RrlImplContaining(im), d_extra_logging(false) { }

    Logger& log();

    void init();

    void released(std::string address);
    void releasedCleaning(std::string address, std::string netmask = "");
    void locked(RrlNode node);

    void error(const std::string& message);
    void error(const std::string& message1, const std::string& message2);

    void info(const std::string& message);
};

class Limits : public RrlImplContaining
{
    std::vector<SingleLimit> d_limits;
    bool              d_limits_enabled;

    SingleLimit initDefaulLimits();

public:
    Limits(RrlIpTableImpl* im) : RrlImplContaining(im), d_limits_enabled(false) { d_limits.push_back(initDefaulLimits()); }

    SingleLimit findLimit(const ComboAddress& address);
    void parseRequestTypes(const string& str, std::set<QType>& types);

    string init(const std::string& fileName);
    int size() const { return d_limits.size(); }
    bool enabled() const { return d_limits_enabled; }
};

struct RrlIpTableImpl
{
  Mode   d_mode;
  u_int8_t  d_ipv4_prefix_length;
  u_int8_t  d_ipv6_prefix_length;

  RrlCleaning d_cleaning;

  u_int32_t     d_request_counter;
  u_int32_t     d_locked_nodes;

  std::set<Netmask> d_white_list;
  bool              d_white_list_enabled;
  RrlMap            d_data;

  Messages d_messages;
  Limits d_limits;

  Time now() const { return boost::posix_time::microsec_clock::local_time(); }
  RrlMap::iterator get(const ComboAddress& addr);
  Netmask truncateAddress(const ComboAddress& addr);

  string parseWhiteList(const string& filename);

  void initialize(bool readStateFromConfig, Mode mode);
  void initCleaningMode();


public:
  RrlIpTableImpl();
  RrlIpTableImpl(Mode mode);
  ~RrlIpTableImpl();

  bool timeToClean() const;
  void cleanRrlNodes();
  bool tryBlock(RrlNode node);
  Mode mode() const { return d_mode; }
  void setMode(Mode mode);
  RrlNode getNode(const ComboAddress& addr);
  bool decreaseCounters(RrlNode &node);

  bool dropQueries() const { return d_mode.type == Mode::Block; }
  bool enabled() const { return d_mode.type != Mode::Off; }

  string reloadWhiteList(const std::string &pathToFile);
  string reloadSpecialLimits(const std::string &pathToFile);
  string information() const;
};
}
#endif // WITH_RRL

#endif // RRLIPTABLEIMPL_HH
