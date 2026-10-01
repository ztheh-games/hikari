#include "hikari/client/game/events/EventListenerDelegate.hpp"

#include <utility>

namespace hikari {

    // Static variable initialization
    long long FunctionDelegateBase::nextId = 0;

    FunctionDelegateBase::FunctionDelegateBase(std::function<void(const EventDataPtr &)> func)
        : id(nextId++)
        , fn(std::move(func))
    {

    }

    void FunctionDelegateBase::operator()(const EventDataPtr & eventPtr) const {
        if(fn) {
            fn(eventPtr);
        }
    }

    bool FunctionDelegateBase::operator == (const FunctionDelegateBase &fdb) const {
        return id == fdb.id;
    }

    bool FunctionDelegateBase::operator != (const FunctionDelegateBase &fdb) const {
        return !(*this==fdb);
    }

} // hikari