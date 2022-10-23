#ifndef WATCHABLE_ITEM_H
#define WATCHABLE_ITEM_H

#include <string>
#include <map>
#include "WatchedEvent.h"
#include <functional>
#include <cstdint>

class Watcher;

/**
 * @brief This is an item which can be watched, each item can have its own mask and handlers.
 *
 */
class WatchableItem
{
public:
    using EventHandlerList = std::vector<std::function<bool(WatchedEvent const & event)>>;

    WatchableItem(std::string path);
    WatchableItem(std::string path, std::string root);

    std::string path() const;
    std::string abspath() const;

    std::string root() const;
    void setRoot(std::string root);
    uint32_t mask() const;

    void setReadonly(bool readonly);
    bool readonly() const;

    void setSource(bool source);
    bool source() const;

    bool match(Watcher& watcher, WatchedEvent event);

    void addEventHandler(uint32_t event, std::function<bool(WatchedEvent const &event)> handler);
    bool handleEvent(WatchedEvent const &event);
    bool handleRelatedEvent(Watcher& watcher, WatchedEvent const &event);

private:
    bool defaultHandler(WatchedEvent const &event);
    bool defaultCreate(WatchedEvent const &event);
    bool defaultWrite(WatchedEvent const &event);

    void _abspath();

private:
    std::string m_path;
    uint32_t m_mask;
    std::map<uint32_t, EventHandlerList> m_eventHandlers;
    bool m_readonly;
    std::string m_root;
    std::string m_abspath;
    bool m_source;
};

using WatchList = std::vector<WatchableItem>;

#endif /* WATCHABLE_ITEM_H */
