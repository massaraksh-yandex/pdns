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

struct Mutex {
    pthread_mutex_t& _m;
    bool locked;

    Mutex(pthread_mutex_t& m) : _m(m), locked(false) { }
    void lock() { pthread_mutex_lock(&_m); locked = true; }
    ~Mutex() { if(locked) pthread_mutex_unlock(&_m); }
};

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

    Type type;

    Mode() : type(Off) {}
    Mode(Type tt) : type(tt) {}

    Mode& operator =(Type tt) { type = tt; return *this; }

    static Mode fromString(const string& str);
    static string toString(Mode mode);
};

inline bool operator ==(Mode m, Mode::Type t) { return m.type == t; }

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

inline Time now() { return boost::posix_time::microsec_clock::local_time(); }

struct InternalNode
{
   Time block_till;
   Time last_request_time;
   u_int64_t counter_ratio;
   u_int64_t counter_types;
   bool blocked;
   pthread_mutex_t mutex;

   void reset() {
       Mutex m(mutex);
       m.lock();

       block_till = Time();
       last_request_time = Time();
       counter_ratio = 0;
       counter_types = 0;
       blocked = false;
   }

   void block(u_int32_t blockinPeriod) {
       Mutex m(mutex);
       m.lock();

       blocked = true;
       block_till = now() + boost::posix_time::milliseconds(blockinPeriod);
   }

   InternalNode() : block_till(), last_request_time(), counter_ratio(0),
     counter_types(0), blocked(false)
   { pthread_mutex_init(&mutex, 0); }

   bool wasLocked() const { return !block_till.is_not_a_date_time(); }
};

//struct Limit {
//    u_int64_t level;
//    u_int64_t max;

//    Limit(u_int64_t m) : max(m) { }
//};

//struct TypeRuleWrapper {
//    QType type;
//    TypeRuleWrapper(const QType& t) : type(t) { }
//};

//struct TypeRule {
//    std::set<QType> types;

//    bool pass(const TypeRuleWrapper& t) const {
//        return types.count(t.type) > 0;
//    }
//};

//struct RatioRuleWrapper {
//    double ratio;
//    TypeRuleWrapper(int requestSize, int responseSize) : ratio(requestSize / responseSize) { }
//};

//struct RatioRule {
//    double ratio;

//    bool pass(const RatioRuleWrapper& r) const {
//        return r.ratio > ratio;
//    }
//};

//struct IntNode {
//    Limit ratio;
//    Limit type;

//    RatioRule ratioRule;
//    TypeRule typeRule;

//    Time block_till;
//    Time last_request_time;
//};

typedef boost::shared_ptr<InternalNode> InternalNodePtr;

//bool tryBlock(RrlNode node);

typedef std::map<Netmask, boost::shared_ptr<InternalNode> > RrlMap;

}

inline bool operator==(const Netmask& a, const Netmask& b)
{
    return a.match(b.getNetwork());
}

inline bool operator<(const Netmask& a, const Netmask& b)
{
    return a.compare(&b.getNetwork());
}

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
