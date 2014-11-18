#ifndef RRL_LOGGER_HH
#define RRL_LOGGER_HH

#include <string>
#include <memory>
#include "rrl_structures.hh"
#include "logger.hh"
#include "iputils.hh"

namespace Rrl {

class Log {
protected:
    static string MessageString;
    static string ErrorString;
    static string LockedString;
    static string ReleasedString;
    static string ReleasedCleaning;

    Log(bool /*extraLogging*/) { }
public:
    virtual ~Log() { }
    virtual void error(const std::string&) = 0;
    virtual void message(const std::string&) = 0;
    virtual void locked(const RrlNode& node) = 0;
    virtual void released(const std::string&, const std::string&) = 0;
    virtual void cleaning(const std::string&) = 0;

    static boost::shared_ptr<Log> make();
};

typedef boost::shared_ptr<Log> LogPtr;

}


#endif // RRL_LOGGER_HH

