#ifndef RRL_CONFIGS_HH
#define RRL_CONFIGS_HH

#include "boost/property_tree/ptree_fwd.hpp"
#include "rrl_structures.hh"
#include "rrl_logger.hh"
#include <set>
#include <vector>

namespace Rrl {

class ConfigReloadable {
protected:
    std::string _errorMsg;
    virtual void parse(boost::property_tree::ptree& tree) = 0;
    virtual void preParse(const std::string& pathToFile, boost::property_tree::ptree& tree) = 0;

    ConfigReloadable(const std::string err) : _errorMsg(err) { }
public:
    virtual ~ConfigReloadable() { }

    std::string reload(const std::string &pathToFile) ;
};

class Limits : public ConfigReloadable {
    std::vector<SingleLimit> _data;
    SingleLimit _default;

    void parseRequestTypes(const std::string &str, std::set<QType> &types);
    void initDefault();

protected:
    void parse(boost::property_tree::ptree& tree);
    void preParse(const std::string& pathToFile, boost::property_tree::ptree& tree);

public:
    Limits();

    SingleLimit get(const ComboAddress &address) const;
    int size() const { return _data.size(); }
};

class Whitelist : public ConfigReloadable {
    std::set<Netmask> _data;

protected:
    void parse(boost::property_tree::ptree &tree);
    void preParse(const std::string &pathToFile, boost::property_tree::ptree &tree);

public:
    Whitelist();

    bool contains(const Netmask &netmask) const;
    int size() const { return _data.size(); }
};
}

#endif // RRL_CONFIGS_HH
