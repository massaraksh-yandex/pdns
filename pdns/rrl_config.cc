#include "rrl_config.hh"

#include "rrl_params.hh"
#include "rrl_functions.hh"

#include <boost/date_time/gregorian/gregorian_io.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree;

namespace Rrl {

std::string ConfigReloadable::reload(const std::string &pathToFile) {
    std::ostringstream error;
    std::string exception;
    using namespace boost::property_tree;
    try {
        ptree tree;

        preParse(pathToFile, tree);
        parse(tree);

        return error.str();
    }
    catch(const ptree_bad_path& err) {
        exception = err.what();
    }
    catch(const ptree_bad_data& err) {
        exception = err.what();
    }
    catch(const file_parser_error& err) {
        exception = err.what();
    }
    catch(const std::exception& err) {
        exception = err.what();
    }
    catch(...) {
        exception = "unknown error";
    }

    error << _errorMsg << ": " << exception;
    return error.str();
}


void Limits::parseRequestTypes(const std::string &str, std::set<QType> &types)
{
    typedef std::vector<std::string> VecString;
    VecString splitted;
    boost::split(splitted, str, boost::is_any_of(","));

    for(VecString::iterator it = splitted.begin(); it != splitted.end(); it++) {
        types.insert(QType(QType::chartocode(it->c_str())));
    }
}

void Limits::initDefault()
{
    _default.netmask = Netmask("0.0.0.0/32");
    _default.limit_types_number = Params::toInt("rrl-limit-types-count");
    _default.limit_ratio_number = Params::toInt("rrl-limit-ratio-count");
    _default.ratio = Params::toDouble("rrl-ratio");
    _default.detection_period = Params::toInt("rrl-detection-period");
    _default.blocking_time = Params::toInt("rrl-blocking-period");

    parseRequestTypes(Params::toString("rrl-types"), _default.types);
}

Limits::Limits(LogPtr log)
    : ConfigReloadable("Special limits for netmasks were not set"),
      _log(log)
{
    const std::string path = Params::toString("rrl-special-limits");
    if(!path.empty()) {
        reload(path);
    }
    initDefault();
}

SingleLimit Limits::get(const ComboAddress &address) const
{
    for (size_t i = 0; i < _data.size(); i++) {
        if (_data[i].netmask.match(address))
            return _data[i];
    }

    return _default;
}

void Limits::parse(boost::property_tree::ptree& tree)
{
    using namespace boost::property_tree;

    std::vector<SingleLimit> newData;
    BOOST_FOREACH (const ptree::value_type& node,
                   tree.get_child("nodes")) {
        SingleLimit limit;
        const ptree& values = node.second;

        limit.netmask = Netmask(values.get<string>("address"));
        limit.limit_types_number = values.get<u_int32_t>("limit-types-count");
        limit.limit_ratio_number = values.get<u_int32_t>("limit-ratio-count");
        parseRequestTypes(values.get<string>("types"), limit.types);
        limit.ratio = values.get<double>("ratio");
        limit.detection_period = values.get<u_int32_t>("detection-period");
        limit.blocking_time = values.get<u_int32_t>("blocking-period");

        newData.push_back(limit);
    }
    _data.swap(newData);
}

void Limits::preParse(const std::string& pathToFile, boost::property_tree::ptree& tree)
{
    info_parser::read_info(pathToFile, tree);
}

Whitelist::Whitelist(LogPtr log)
    : ConfigReloadable("Whitelist was not set"), _log(log)
{
    std::string path = Params::toString("rrl-enable-white-list");
    if(!path.empty()) {
        std::string status = reload(path);
        if(!status.empty()) {
            _log->message(status);
        }
    }
}

bool Whitelist::contains(const Netmask &netmask) const
{
    return _data.count(netmask);
}

void Whitelist::preParse(const std::string &pathToFile, boost::property_tree::ptree &tree) {
    info_parser::read_info(pathToFile, tree);
}

void Whitelist::parse(boost::property_tree::ptree &tree) {
    std::set<Netmask> newData;

    BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                   tree.get_child("white_list")) {
        newData.insert(node.second.get_value<string>());
    }
    _data.swap(newData);
}



}
