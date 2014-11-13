#include "rrl_logger.hh"
#include "rrl_params.hh"

namespace Rrl {

string Log::MessageString    = "[message]";
string Log::ErrorString      = "[error]";
string Log::LockedString     = "[locked]";
string Log::ReleasedString   = "[released]";
string Log::ReleasedCleaning = "[cleaning]";

struct SilentLogger : public Log
{
    SilentLogger() : Log(false) { }
    void error(const std::string&, const std::string& msg2) { }
    void message(const std::string&) { }
    void locked(const RrlNode& node) { }
    void released(const std::string&, const std::string&) { }
    void cleaning(const std::string&) { }
};

class UsualLogger : public Log {
protected:
    bool extra;
    std::auto_ptr<Logger> logger;
    virtual Logger& log() { return theL(); }
public:
    UsualLogger(const std::string& name, bool extraLogging)
        :  Log(extraLogging), extra(extraLogging), logger(new Logger(name)) {
    }

    void error(const std::string& msg1, const std::string& msg2) {
        log() << ErrorString << " " << msg1 << " " << msg2 << "\n";
    }

    void message(const std::string& msg) {
        log() << MessageString << " " << msg << "\n";
    }

    void locked(const RrlNode& node) {
        log() << LockedString << " address: " << node.address.toString()
              << ", limit netmask: " << node.limit.netmask.toString();

        if(extra) {
            Rrl::InternalNode& intNode = *node.node;
            log() << "; Ratio-requests counter: " << intNode.counter_ratio
                  << "; Type-requests counter: " << intNode.counter_types
                  << "; Ratio limit: " << node.limit.limit_ratio_number
                  << "; Types limit: " << node.limit.limit_types_number;
        }
        log() << "\n";
    }

    void released(const std::string& addr, const std::string& limitNetmask) {
        log() << ReleasedString << " address: " << addr
              << ", limit netmask: " << limitNetmask << "\n";
    }

    void cleaning(const std::string& addr) {
        log() << ReleasedString << " address: " << addr << "\n";
    }
};

struct CustomFileLogger : public UsualLogger {
    CustomFileLogger(const std::string& name, const std::string& filename, bool extraLogging)
        : UsualLogger(name, extraLogging) {
        logger->toFile(filename);
    }
};

std::auto_ptr<Log> Log::make()
{
    bool extra = Params::toBool("rrl-enable-extra-logging");
    switch(type) {
    case Off:
        return std::auto_ptr<Log>(new SilentLogger());
        ;break;
    case Usual:
        return std::auto_ptr<Log>(new UsualLogger("rrl-log", extra));
        ;break;
    case CustomFile:
        const std::string name = Params::toString("rrl-log-file");
        return std::auto_ptr<Log>(new CustomFileLogger("rrl-log", name, extra));
        break;
    }
}

}
