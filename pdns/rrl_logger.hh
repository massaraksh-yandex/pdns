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

public:
    Log(bool extraLogging) { }
    virtual ~Log() { }
    virtual void error(const std::string&, const std::string& msg2) = 0;
    virtual void message(const std::string&) = 0;
    virtual void locked(const RrlNode& node) = 0;
    virtual void released(const std::string&, const std::string&) = 0;
    virtual void cleaning(const std::string&) = 0;

    enum LogType {
        Off,
        Usual,
        CustomFile
    };

    std::auto_ptr<Log> make();
};

}


#endif // RRL_LOGGER_HH

