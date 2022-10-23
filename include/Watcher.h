#ifndef WATCHER_H
#define WATCHER_H

#define DEFAULT_MAXIMUM_WATCHED_FILE_DESCRIPTORS 1024

#include <cstdlib>
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <map>

#include "WatchableItem.h"

class WatchedEvent;
using WatchedQueue = std::queue<WatchedEvent>;

/**
 * @brief A watched item is a watchable item that is being watched using inotify.
 *
 */
class WatchedItem
{
public:
  WatchedItem(Watcher &watcher, WatchableItem watchableItem, uint32_t watchedDescriptor);
  ~WatchedItem() = default;
  WatchedItem(WatchedItem const &other) = default;
  WatchedItem &operator=(WatchedItem const &other);

  std::string path() const;

  bool handleEvent(WatchedEvent event);
  bool handleRelatedEvent(WatchedEvent event);

  WatchableItem const &watchableItem() const;

  int descriptor() const;

private:
  Watcher &m_watcher;
  WatchableItem m_watchableItem;
  int m_watchedDescriptor;
};

using WatchedList = std::vector<WatchedItem>;

class Watcher
{
public:
  Watcher();

  ~Watcher();

  void setMaximumWatchedFileDescriptors(size_t n);

  bool watching() const;

  void start(WatchList const watchList, bool sync = false, int testMode=0);

  void stop();

  WatchableItem const &getWatchableItem(int descriptor) const;

private:
  void _open_inotify();

  void _process_inotify_events();

  int _event_check();

  int _read_events();
  void _handle_events();

  void _watchItem(WatchableItem item);
  void _watchItems();
  void _matchItems();
  void _sync();

private:
  int m_inotifyFileDescriptor;
  bool m_keepRunning;
  WatchedQueue m_eventQueue;
  WatchList m_watchList;
  WatchedList m_watchedList;
  size_t m_maximumWatchedFileDescriptors;
  std::multimap<uint32_t, std::reference_wrapper<WatchedItem> > m_copyTargets;
  int m_testmode;
  bool m_sync;
};

#endif /* WATCHER_H */
