#include "rrl_limits.hh"
#include "rrl_params.hh"

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

Limits::Limits()
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

string Limits::reload(const std::string &pathToFile)
{
    std::ostringstream error;
    try
    {
        std::vector<SingleLimit> newLimits;

        ptree tree;
        info_parser::read_info(pathToFile, tree);

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

            newLimits.push_back(limit);
        }
        _data.swap(newLimits);
        return error.str();
    }
    catch(const ptree_bad_path& err) {
        error << err.what();
    }
    catch(const ptree_bad_data& err) {
        error << err.what();
    }
    catch(const info_parser::info_parser_error& err) {
        error << err.what();
    }
    catch(...) {
        error << "unknown error";
    }

    error << "Special limits for netmasks were not set. Reason: " << error;
    return error.str();
}

}
