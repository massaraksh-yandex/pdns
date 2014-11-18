#ifndef RRLIPTABLESTRUCTURES_HH
#define RRLIPTABLESTRUCTURES_HH

#include "config-recursor.h"
#ifdef WITH_RRL

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/gregorian/gregorian_io.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <set>
#include <map>
#include "iputils.hh"

namespace Rrl {

class AddressUtils {
    u_int8_t  ipv4_prefix_length;
    u_int8_t  ipv6_prefix_length;
public:
    AddressUtils();

    Netmask truncate(const ComboAddress& addr);
};

struct Mode {
    enum Type {
        Off,
        LogOnly,
        Truncate,
        Block
    };

    Mode(Type tt = Off) : type(tt) {}

    operator Mode() { return type; }

    static Mode fromString(const string& str);
    static string toString(Mode mode);

private:
    Type type;
};

inline bool operator ==(Mode m, Mode::Type t) { return m == t; }
inline bool operator !=(Mode m, Mode::Type t) { return m != t; }

typedef boost::posix_time::ptime Time;

struct SingleLimit {
  Netmask netmask;

  u_int32_t limit_ratio_number;
  u_int32_t limit_types_number;

  std::set<QType> types;
  double ratio;

  u_int32_t detection_period;
  u_int32_t blocking_time;
};

struct InternalNode
{
   Time block_till;
   Time last_request_time;
   u_int64_t counter_ratio;
   u_int64_t counter_types;
   bool blocked;
   pthread_mutex_t mutex;

   void reset();
   void block(u_int32_t blockinPeriod);

   InternalNode() : block_till(), last_request_time(), counter_ratio(0),
     counter_types(0), blocked(false)
   { pthread_mutex_init(&mutex, 0); }

   bool wasLocked() const { return !block_till.is_not_a_date_time(); }
};
typedef boost::shared_ptr<InternalNode> InternalNodePtr;

}

class Locker {
    pthread_mutex_t& _m;
    bool locked;
public:

    Locker(pthread_mutex_t& m) : _m(m), locked(false) { }
    void lock() { pthread_mutex_lock(&_m); locked = true; }
    ~Locker() { if(locked) pthread_mutex_unlock(&_m); }
};

struct RrlNode
{
  bool isInWhiteList;
  ComboAddress address;
  Rrl::InternalNodePtr node;
  Rrl::SingleLimit limit;

  bool checkState() const;

public:
  RrlNode(Rrl::InternalNodePtr it, const ComboAddress& add, bool inWhiteList, const Rrl::SingleLimit& lim)
    : isInWhiteList(inWhiteList), address(add), node(it), limit(lim)
  { }

  bool update(QType type);
  bool update(double ratio);

  bool blocked() const;

  bool valid() const { return (bool)node; }
};
#endif // WITH_RRL

#endif // RRLIPTABLESTRUCTURES_HH
