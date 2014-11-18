#include "rrl_logger.hh"
#include "rrl_params.hh"

namespace Rrl {

LogPtr Log::_global;
string Log::MessageString    = "[message]";
string Log::ErrorString      = "[error]";
string Log::LockedString     = "[locked]";
string Log::ReleasedString   = "[released]";
string Log::ReleasedCleaning = "[cleaning]";

struct SilentLogger : public Log
{
    SilentLogger() : Log(false) { }
    void error(const std::string&) { }
    void message(const std::string&) { }
    void locked(const RrlNode&) { }
    void released(const std::string&, const std::string&) { }
    void cleaning(const std::string&) { }
};

class ColsoleLogger : public Log {
protected:
    bool extra;
    std::auto_ptr<Logger> logger;
    virtual Logger& log() { return *logger; }
public:
    ColsoleLogger(const std::string& name, bool extraLogging)
        :  Log(extraLogging), extra(extraLogging), logger(new Logger(name)) {
    }

    void error(const std::string& msg) {
        log() << ErrorString << " " << msg << "\n";
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
                  << "; Ratio limit: " << node.limit.ratio.limit
                  << "; Types limit: " << node.limit.types.limit;
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

struct FileLogger : public ColsoleLogger {
    FileLogger(const std::string& name, const std::string& filename, bool extraLogging)
        : ColsoleLogger(name, extraLogging) {
        logger->toFile(filename);
    }
};

boost::shared_ptr<Log> Log::make()
{
    bool extra = Params::toBool("rrl-enable-extra-logging");
    std::string type = Params::toString("rrl-logging");

    if(type == "off") {
        _global.reset(new SilentLogger());
    } else if(type == "console") {
        _global.reset(new ColsoleLogger("rrl-log", extra));
    } else if(type == "file") {
        std::string name = Params::toString("rrl-log-file");
        _global.reset(new FileLogger("rrl-log", name, extra));
    } else {
        theL() << Logger::Error << " unknown type of rrl logger = " << type << ". All messages will be logged by default logger.";
        _global.reset(new ColsoleLogger("rrl-log", extra));
    }

    return _global;
}

Log &Log::log() {
    if(!_global.get())
        return *make();

    return *_global;
}

}
