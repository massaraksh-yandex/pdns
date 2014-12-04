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
    std::string _prefix;
    virtual Logger& log() { return theL(); }
public:
    ColsoleLogger(const std::string& prefix, bool extraLogging)
        :  Log(extraLogging), _prefix(prefix) {
    }

    void error(const std::string& msg) {
        log() << _prefix << ErrorString << " " << msg << std::endl;
    }

    void message(const std::string& msg) {
        log() << _prefix << MessageString << " " << msg << std::endl;
    }

    void locked(const RrlNode& node) {
        log() << _prefix << LockedString << " address: " << node.address.toString()
              << ", limit netmask: " << node.limit.netmask.toString();

        if(_extraLogging) {
            Rrl::InternalNode& intNode = *node.node;
            log() << "; Ratio-requests counter: " << intNode.counter_ratio
                  << "; Type-requests counter: " << intNode.counter_types
                  << "; Ratio limit: " << node.limit.ratio.limit
                  << "; Types limit: " << node.limit.types.limit;
        }
        log() << std::endl;
    }

    void released(const std::string& addr, const std::string& limitNetmask) {
        log() << _prefix << ReleasedString << " address: " << addr
              << ", limit netmask: " << limitNetmask << std::endl;
    }

    void cleaning(const std::string& addr) {
        log() << _prefix << ReleasedCleaning << " address: " << addr << std::endl;
    }
};

struct FileLogger : public ColsoleLogger {
protected:
    std::auto_ptr<Logger> logger;
    Logger& log() { return *logger; }

public:
    FileLogger(const std::string& name, const std::string& filename, bool extraLogging)
        : ColsoleLogger("", extraLogging), logger(new Logger(name)) {
        bool res = logger->toFile(filename);
        if(!res) {
            theL() << Logger::Error << "[rrl-log] cannot open file " << filename << std::endl;
        }
    }
};

boost::shared_ptr<Log> Log::make()
{
    bool extra = Params::toBool("rrl-enable-extra-logging");
    std::string type = Params::toString("rrl-logging");

    if(type == "off") {
        _global.reset(new SilentLogger());
    } else if(type == "console") {
        _global.reset(new ColsoleLogger("[rrl-log] ", extra));
    } else if(type == "file") {
        std::string name = Params::toString("rrl-log-file");
        _global.reset(new FileLogger("rrl-log", name, extra));
    } else {
        theL() << Logger::Error << "[rrl-log] unknown type of rrl logger = " << type << ". All messages will be logged by default logger.";
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
