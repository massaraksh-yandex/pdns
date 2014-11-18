#include "rrl_whitelist.hh"
#include "rrl_params.hh"

#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree;

namespace Rrl {

Whitelist::Whitelist()
{
    std::string path = Params::toString("rrl-enable-white-list");
    if(!path.empty()) {
        reload(path);
    }
}

bool Whitelist::contains(const Netmask &netmask) const
{
    return _data.count(netmask);
}

string Whitelist::reload(const std::string &pathToFile)
{
    std::ostringstream error;
    try
    {
        using boost::property_tree::ptree;
        ptree tree;
        boost::property_tree::info_parser::read_info(pathToFile, tree);
        std::set<Netmask> newWhiteList;

        BOOST_FOREACH (const boost::property_tree::ptree::value_type& node,
                       tree.get_child("white_list")) {

            newWhiteList.insert(node.second.get_value<string>());
        }
        _data.swap(newWhiteList);
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
        error << "Unknown exception";
    }

    error << "White list was not set. Reason: " << error;
    return error.str();
}

}
