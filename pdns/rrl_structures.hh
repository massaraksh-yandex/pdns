#ifndef RRLIPTABLESTRUCTURES_HH
#define RRLIPTABLESTRUCTURES_HH

#include "config-recursor.h"
#ifdef WITH_RRL

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/make_shared.hpp>
#include <set>
#include <map>
#include "iputils.hh"

namespace Rrl {
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

struct InternalNode
{
   Time block_till;
   Time last_request_time;
   u_int64_t counter_ratio;
   u_int64_t counter_types;
   bool blocked;

   InternalNode() : block_till(), last_request_time(), counter_ratio(0),
     counter_types(0), blocked(false)
   { }
};
typedef boost::shared_ptr<InternalNode> InternalNodePtr;

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
