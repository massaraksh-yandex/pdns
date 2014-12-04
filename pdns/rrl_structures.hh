#ifndef RRLIPTABLESTRUCTURES_HH
#define RRLIPTABLESTRUCTURES_HH

#include "config.h"
#ifdef WITH_RRL

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/make_shared.hpp>
#include "iputils.hh"
#include <set>

namespace Rrl {

typedef boost::posix_time::ptime Time;
typedef boost::posix_time::time_duration TimeDiff;

struct InternalNode {
    Time at_least_block_till;
    Time last_request_time;
    u_int64_t counter_ratio;
    u_int64_t counter_types;
    bool blocked;
    pthread_mutex_t mutex;

    void reset();
    void block(u_int32_t blockinPeriod, const Time &now);

    InternalNode() : at_least_block_till(), last_request_time(), counter_ratio(0),
        counter_types(0), blocked(false)
    { pthread_mutex_init(&mutex, 0); }

    bool wasLocked() const;

    std::string toString() const {
        std::ostringstream str;
#define print(name) "\"" #name "\": \"" << name << "\""
        str << "{ " << std::boolalpha
            << print(at_least_block_till) << ", "
            << print(last_request_time) << ", "
            << print(counter_ratio) << ", "
            << print(counter_types) << ", "
            << print(blocked) << " }";
        return str.str();
#undef print
    }
};
typedef boost::shared_ptr<InternalNode> InternalNodePtr;

struct SingleLimit {
    struct LimitType {
        std::set<QType> value;
        u_int32_t limit;

        bool operator()(QType val) { return value.count(val) > 0; }
    };

    struct LimitRatio {
        double value;
        u_int32_t limit;

        bool operator()(double val) { return val > value; }
    };

    Netmask netmask;
    u_int32_t detection_period;
    u_int32_t blocking_time;

    LimitType types;
    LimitRatio ratio;
};

}

struct PackageInfo {
    QType _type;
    double _ratio;

    PackageInfo() : _ratio(0.0) { }

    void setType(const QType& type) { _type = type; }
    void setRatio(double response, double request) { _ratio = response / request; }
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

    void update(const PackageInfo& info);
    bool blocked() const;
    bool valid() const { return (bool)node; }
};
#endif // WITH_RRL

#endif // RRLIPTABLESTRUCTURES_HH
