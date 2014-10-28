#include "rrl.hh"
#include "rrl_impl.hh"

#ifdef WITH_RRL

struct Mutex {
    pthread_mutex_t& _m;

    Mutex(pthread_mutex_t& m) : _m(m) { pthread_mutex_lock(&_m); }
    ~Mutex() { pthread_mutex_unlock(&_m); }
};

RrlIpTable::RrlIpTable() {
    d_impl.reset(new Rrl::RrlIpTableImpl());
    pthread_mutex_init(&d_lock, 0);
}

RrlNode RrlIpTable::getNode(const ComboAddress &addr) {
    RrlNode node = d_impl->getNode(addr);
    d_impl->decreaseCounters(node);

    return node;
}

RrlNode RrlIpTable::getNodeAndLock(const ComboAddress &addr) {
    Mutex mutex(d_lock);

    return getNode(addr);
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
        d_impl->log() << Logger::Alert << Rrl::RrlIpTableImpl::rrlMessageString << str.str() << std::endl;
        return str.str() + "\n";
    }

    Mutex mutex(d_lock);
    string res = d_impl->reloadWhiteList(*begin);

    return res;
}

std::string RrlIpTable::reloadSpecialLimits(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) {
    std::ostringstream str;
    if((end - begin) != 1) {
        str << "Trying to reload special limits but passed wrong params list. size == " << (end - begin);
        d_impl->log() << Logger::Alert << Rrl::RrlIpTableImpl::rrlMessageString << str.str() << std::endl;
        return str.str() + "\n";
    }

    Mutex mutex(d_lock);
    string res = d_impl->reloadSpecialLimits(*begin);

    return res;
}

std::string RrlIpTable::setRrlMode(std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end) {
    std::ostringstream str;
    if((end - begin) != 1) {
        str << "Trying to reset rrl mode but passed wrong params list. size == " << (end - begin);
        d_impl->log() << Logger::Alert << Rrl::RrlIpTableImpl::rrlMessageString << str.str() << std::endl;
        return str.str() + "\n";
    }

    Rrl::Mode newMode;
    try {
        newMode = Rrl::Mode::fromString(*begin);
    }
    catch(std::exception& ex)
    {
        str << "Trying to reset rrl mode but passed wrong param == " << *begin;
        d_impl->log() << Logger::Alert << Rrl::RrlIpTableImpl::rrlMessageString << str.str() << std::endl;
        return str.str() + "\n";
    }

    str << "Complete. New mode: " << Rrl::Mode::toString(newMode) << "; Old mode: "
        << Rrl::Mode::toString(d_impl->mode()) << "\n";

    Mutex mutex(d_lock);
    if(d_impl->mode() == Rrl::Mode::Off || newMode == Rrl::Mode::Off)
        d_impl.reset(new Rrl::RrlIpTableImpl(newMode));
    else
        d_impl->setMode(newMode);

    return str.str();
}

std::string RrlIpTable::information() const {
    return d_impl->information();
}

bool RrlNode::checkState() const {
    return rrlIpTable().d_impl->mode().type != Rrl::Mode::LogOnly;
}

bool RrlNode::update(QType type) {
    RrlIpTable& table = rrlIpTable();
    if(!table.enabled() || !valid())
      return false;

    node->last_request_time = boost::posix_time::microsec_clock::local_time();
    if(limit.types.count(type) && valid()) {
      node->counter_types++;
      node->blocked = table.d_impl->tryBlock(*this);
      return node->blocked && checkState();
    }
    return false;
}

bool RrlNode::update(double ratio) {
    RrlIpTable& table = rrlIpTable();
    if(!table.enabled() || !valid())
      return false;

    node->last_request_time = boost::posix_time::microsec_clock::local_time();
    if(ratio >= limit.ratio) {
      node->counter_ratio++;
      node->blocked = table.d_impl->tryBlock(*this);
      return node->blocked && checkState();
    }
    return false;
}

bool RrlNode::blocked() const {
    if(!rrlIpTable().enabled() || isInWhiteList)
      return false;

    bool a = checkState();

    return node->blocked && a;
}

namespace Rrl {
void cleanRrlCache(void*) {
    rrlIpTable().d_impl->cleanRrlNodes();
}

}
#endif // WITH_RRL
