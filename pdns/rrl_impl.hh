#include "config.h"
#ifndef RRLIPTABLEIMPL_HH
#define RRLIPTABLEIMPL_HH

#ifdef WITH_RRL

#include "rrl_structures.hh"
#include "rrl_map.hh"
#include "rrl_cleaning.hh"
#include "rrl_logger.hh"
#include "rrl_functions.hh"
#include "rrl_config.hh"

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
    bool operator ==(Mode::Type t) const { return type == t; }
    bool operator !=(Mode::Type t) const { return type != t; }

    static Mode fromString(const string& str);
    static string toString(Mode mode);

private:
    Type type;
};

class RrlIpTableImpl {
    Map _map;
    Limits _limits;
    Whitelist _whitelist;
    AddressUtils _addressUtils;
    Mode _mode;

    CleaningPtr _cleaning;

public:
    RrlIpTableImpl();
    RrlIpTableImpl(Mode mode);

    RrlNode getNode(const ComboAddress& addr);

    bool dropQueries() const { return _mode == Mode::Block; }
    bool enabled() const { return _mode != Mode::Off; }

    bool timeToClean() const {
        if(enabled())
            return _cleaning->time();
        else
            return false;
    }

    const Map& map() const { return _map; }

    void cleanRrlNodes() {
        _cleaning->clean();
    }

    Mode mode() const { return _mode; }

    string reloadWhitelist(const std::string &pathToFile);
    string reloadSpecialLimits(const std::string &pathToFile);
    string setMode(Mode mode);
    string information() const;
    string dbDump();
};

}
#endif // WITH_RRL

#endif // RRLIPTABLEIMPL_HH
