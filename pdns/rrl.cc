#include "rrl.hh"
#include "rrl_impl.hh"
#include "rrl_functions.hh"

#ifdef WITH_RRL

RrlIpTable::RrlIpTable() {
    d_impl.reset(new Rrl::RrlIpTableImpl());
    pthread_mutex_init(&d_lock, 0);
}

RrlNode RrlIpTable::getNode(const ComboAddress &addr) {
    return d_impl->getNode(addr);
}

bool RrlIpTable::dropQueries() const {
    return d_impl->dropQueries();
}

bool RrlIpTable::enabled() const {
    return d_impl->enabled();
}

bool RrlIpTable::timeToClean() const {
    return d_impl->timeToClean();
}

std::string RrlIpTable::reloadWhiteList(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) {
    std::ostringstream str;
    if((end - begin) != 1) {
        str << "Trying to reload white list but passed wrong params list. size == " << (end - begin);
        Rrl::Log::log().error(str.str());

        return str.str() + "\n";
    }

    Locker mutex(d_lock);
    string res = d_impl->reloadWhitelist(*begin);

    return res;
}

std::string RrlIpTable::reloadSpecialLimits(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) {
    std::ostringstream str;
    if((end - begin) != 1) {
        str << "Trying to reload special limits but passed wrong params list. size == " << (end - begin);
        Rrl::Log::log().error(str.str());
        return str.str() + "\n";
    }

    Locker mutex(d_lock);
    string res = d_impl->reloadSpecialLimits(*begin);

    return res;
}

std::string RrlIpTable::setRrlMode(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) {
    std::ostringstream str;
    if((end - begin) != 1) {
        str << "Trying to reset rrl mode but passed wrong params list. size == " << (end - begin);
        Rrl::Log::log().error(str.str());
        return str.str() + "\n";
    }

    Rrl::Mode newMode;
    try {
        newMode = Rrl::Mode::fromString(*begin);
    }
    catch(std::exception&)
    {
        str << "Trying to reset rrl mode but passed wrong param == " << *begin;
        Rrl::Log::log().error(str.str());
        return str.str() + "\n";
    }

    str << "Complete. New mode: " << Rrl::Mode::toString(newMode) << "; Old mode: "
        << Rrl::Mode::toString(d_impl->mode()) << "\n";

    Locker mutex(d_lock);
    d_impl->setMode(newMode);

    return str.str();
}

std::string RrlIpTable::information() const {
    return d_impl->information();
}

namespace Rrl {
void cleanRrlCache(void*) {
    rrlIpTable().d_impl->cleanRrlNodes();
}

}
#endif // WITH_RRL
