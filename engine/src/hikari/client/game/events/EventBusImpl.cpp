#include "hikari/client/game/events/EventBusImpl.hpp"
#include "hikari/client/game/events/EventData.hpp"
#include "hikari/core/util/Log.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace hikari {

    EventBusImpl::EventBusImpl(const std::string & name, bool setAsGlobal)
        : EventBus(name, setAsGlobal)
    {
        activeQueueIndex = 0;
    }

    EventBusImpl::~EventBusImpl() {
        // Do nothing!
    }

    bool EventBusImpl::addListener(const EventListenerDelegate & eventDelegate, const EventType & type) {
        // This will create a list of one doesn't exist.
        EventListenerList & eventListenerList = eventListeners[type];

        for(auto it = std::begin(eventListenerList); it != std::end(eventListenerList); ++it) {
            if(eventDelegate == (*it)) {
                HIKARI_LOG(error) << "Attempting to double-register a delegate.";
                return false;
            }
        }

        eventListenerList.push_back(eventDelegate);

        return true;
    }

    bool EventBusImpl::removeListener(const EventListenerDelegate & eventDelegate, const EventType & type) {
        bool success = false;

        auto findIt = eventListeners.find(type);

        if(findIt != std::end(eventListeners)) {

            EventListenerList & listeners = findIt->second;

            for(auto it = std::begin(listeners); it != std::end(listeners); ++it) {
                if(eventDelegate == (*it)) {
                    listeners.erase(it);
                    success = true;
                    break;
                }
            }
        }

        return success;
    }
    
    bool EventBusImpl::triggerEvent(const EventDataPtr & event) const {
        bool processed = false;

        auto findIt = eventListeners.find(event->getEventType());

        if(findIt != std::end(eventListeners)) {
            const EventListenerList & eventListenerList = findIt->second;

            for(auto it = eventListenerList.cbegin(); it != eventListenerList.cend(); ++it) {
                // Copy so the delegate survives if it removes itself while running.
                const EventListenerDelegate listener = (*it);

                listener(event);

                processed = true;
            }
        }

        return processed;
    }
    
    bool EventBusImpl::queueEvent(const EventDataPtr & event) {
        // ASSERT activeQueueIndex >= 0
        // ASSERT activeQueueIndex < 2
        
        auto findIt = eventListeners.find(event->getEventType());

        if(findIt != std::end(eventListeners)) {
            eventQueues[activeQueueIndex].push_back(event);
            return true;
        }

        return false;
    }
    
    bool EventBusImpl::cancelEvent(const EventType & type, bool allOfType) {
        // ASSERT activeQueueIndex >= 0
        // ASSERT activeQueueIndex < 2
        
        bool success = false;

        auto findIt = eventListeners.find(type);

        if(findIt != std::end(eventListeners)) {
            EventQueue & eventQueue = eventQueues[activeQueueIndex];

            const auto isOfType = [&type](const EventDataPtr & event) {
                return event->getEventType() == type;
            };

            if(allOfType) {
                const auto newEnd = std::remove_if(std::begin(eventQueue), std::end(eventQueue), isOfType);
                success = newEnd != std::end(eventQueue);
                eventQueue.erase(newEnd, std::end(eventQueue));
            } else {
                const auto it = std::find_if(std::begin(eventQueue), std::end(eventQueue), isOfType);

                if(it != std::end(eventQueue)) {
                    eventQueue.erase(it);
                    success = true;
                }
            }
        }

        return success;
    }
    
    bool EventBusImpl::processEvents(unsigned long maxMillis) {
        int queueToProcessIndex = activeQueueIndex;

        // Swap active queue and clear the new queue
        activeQueueIndex = (activeQueueIndex + 1) % 2;
        eventQueues[activeQueueIndex].clear();

        // Process queued events
        while(!eventQueues[queueToProcessIndex].empty()) {
            EventDataPtr event = std::move(eventQueues[queueToProcessIndex].front());
            eventQueues[queueToProcessIndex].pop_front();

            const EventType & eventType = event->getEventType();

            // Find all delegate functions for this event
            auto findIt = eventListeners.find(eventType);

            if(findIt != std::end(eventListeners)) {
                const EventListenerList & listeners = findIt->second;

                // Call each listener
                for(auto it = std::begin(listeners); it != std::end(listeners); ++it) {
                    const EventListenerDelegate listener = (*it);
                    listener(event);
                }
            }

            // TODO: Add elapsed processing time here so we can bail out if
            // event processing is taking too much time in one frame.
        }

        // If we couldn't process all events this frame, put them in the other
        // queue so they'll get processed next frame.
        bool queueFlushed = eventQueues[queueToProcessIndex].empty();

        if(!queueFlushed) {
            while(!eventQueues[queueToProcessIndex].empty()) {
                auto event = std::move(eventQueues[queueToProcessIndex].back());
                eventQueues[queueToProcessIndex].pop_back();
                eventQueues[activeQueueIndex].push_front(std::move(event));
            }
        }

        return queueFlushed;
    }
    

} // hikari