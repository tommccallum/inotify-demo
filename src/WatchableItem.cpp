#include <iostream>
#include <cstring>
#include <sys/inotify.h>
#include <unistd.h>
#include <climits>
#include <iomanip>
#include "File.h"
#include "WatchableItem.h"
#include "INotify.h"
#include "Watcher.h"


/**
 * @brief This is an item which can be watched, each item can have its own mask and handlers.
 *
 */
WatchableItem::WatchableItem(std::string path) : m_path(path), m_mask(IN_ALL_EVENTS), m_readonly(false), m_root(""), m_source(false)
{
    _abspath();
};
WatchableItem::WatchableItem(std::string path, std::string root) : m_path(path), m_mask(IN_ALL_EVENTS), m_readonly(false), m_root(root), m_source(false)
{
    _abspath();
};

void WatchableItem::_abspath()
{
    char buffer[PATH_MAX];
    memset(buffer, 0, PATH_MAX);
    realpath(m_path.c_str(), buffer);
    m_abspath.assign(buffer, strlen(buffer));
}

std::string WatchableItem::path() const
{
    return m_path;
}

std::string WatchableItem::abspath() const
{
    return m_abspath;
}

uint32_t WatchableItem::mask() const
{
    return m_mask;
}

void WatchableItem::setReadonly(bool readonly)
{
    m_readonly = readonly;
}

bool WatchableItem::readonly() const
{
    return m_readonly;
}

void WatchableItem::setSource(bool source)
{
    m_source = source;
}

bool WatchableItem::source() const
{
    return m_source;
}

bool WatchableItem::match(Watcher& watcher, WatchedEvent event)
{
    // we are here because this event does not have the same Descriptor but
    // may be we still want to receive the event.
    // auto &watchableItem = watcher->getWatchableItem(event.watchedDescriptor);

    // if different root but same basename then
    // we might want to copy/save this.
    return false;
}

bool WatchableItem::handleRelatedEvent(Watcher& watcher, WatchedEvent const & event) {
    if ( readonly() ) {
        return false;
    }
    if ( source() ) {
        return false;
    }
    auto sourceItem = watcher.getWatchableItem(event.watchedDescriptor);
    File f(sourceItem.abspath());
    File g(abspath());
    if ( f.isDirectory() == false && g.isDirectory() == false ) {
        if ( event.eventType & IN_CLOSE_WRITE || event.eventType & IN_MODIFY ) {
            std::cout << "MODIFY " << f.abspath() << " => " << g.abspath() << std::endl;
            f.copyTo(abspath());
        } 
    } else if ( f.isDirectory() == true && g.isDirectory() == true ) {
        if ( event.eventType & IN_CREATE ) {
            std::string newFilePath = f.dirname() + f.separator() + event.name;
            std::string destFilePath = g.dirname() + g.separator() + event.name;
            std::cout << "IN_CREATE " << newFilePath << " => " << destFilePath << std::endl;
            File h(newFilePath);
            h.copyTo(destFilePath);
        }
    }
    return true;
}

void WatchableItem::addEventHandler(uint32_t event, std::function<bool(WatchedEvent const &event)> handler)
{
    auto existingList = m_eventHandlers.find(event);
    if (existingList == m_eventHandlers.end())
    {
        m_eventHandlers.insert(std::make_pair(event, EventHandlerList{handler}));
    }
    else
    {
        existingList->second.push_back(handler);
    }
}

bool WatchableItem::handleEvent(WatchedEvent const &event)
{
    auto handlerList = m_eventHandlers.find(event.mask);
    bool success = true;
    if (handlerList == m_eventHandlers.end() || handlerList->second.size() == 0)
    {
        if (event.eventType & IN_ACCESS || event.eventType & IN_CLOSE_NOWRITE)
        {
            // Ignore, as nothing changes here.  Any program which accesses the directory will
            // cause one of these.
        }
        else
        {
            if (event.eventType & IN_CREATE)
            {
                success &= defaultCreate(event);
            }
            else if (event.eventType & IN_CLOSE_WRITE || event.eventType & IN_MODIFY)
            {
                success &= defaultWrite(event);
            }
            else
            {
                success &= defaultHandler(event);
            }
        }
    }
    else
    {
        for (auto &usedDefinedHandler : handlerList->second)
        {
            success &= usedDefinedHandler(event);
            if (!success)
            {
                break;
            }
        }
    }
    return success;
}

bool WatchableItem::defaultHandler(WatchedEvent const &event)
{
    std::cout << "Default handler for " << m_abspath << ": "
              << inotifyEvents.find(event.eventType)->second
              << " (0x"
              << std::hex << std::setw(8) << std::setfill('0')
              << event.eventType
              << ")" << std::endl;
    std::cout << " - Event Name: " << event.name << std::endl;
    std::cout << " - Is Directory: " << event.type << std::endl;
    return true;
}

bool WatchableItem::defaultCreate(WatchedEvent const &event)
{
    std::cout << "CREATE Event: " << event.name << " (IsDir: " << event.type << ")" << std::endl;
    return true;
}

bool WatchableItem::defaultWrite(WatchedEvent const &event)
{
    if (m_readonly)
    {
        std::cout << "Ignoring event for " << m_path << " as this is readonly." << std::endl;
        return true;
    }
    std::cout << "Copy-on-write event detected" << std::endl;
    return true;
}

std::string WatchableItem::root() const {
    return m_root;
}

void WatchableItem::setRoot(std::string root) {
    File f(root);
    if (f.isDirectory() == false) {
        m_root = f.dirname();
    } else {
        m_root = root;
    }
}