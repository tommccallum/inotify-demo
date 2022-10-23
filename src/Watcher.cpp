#include <iostream>
#include <iomanip>
#include <errno.h>
#include <poll.h>
#include <cstdio>
#include <cstdlib>
#include <sys/inotify.h>
#include <unistd.h>
#include <signal.h>
#include <queue>
#include <string>
#include <iostream>
#include <memory>
#include <cstring>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include <openssl/evp.h>
#include <errno.h>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <ftw.h>
#include <set>

#include "BoolOrError.h"
#include "WatchedEvent.h"
#include "Watcher.h"
#include "File.h"

WatchedItem::WatchedItem(Watcher &watcher, WatchableItem watchableItem, uint32_t watchedDescriptor) : m_watcher(watcher), m_watchableItem(watchableItem), m_watchedDescriptor(watchedDescriptor) {}

WatchedItem &WatchedItem::operator=(WatchedItem const &other)
{
  m_watcher = other.m_watcher;
  m_watchableItem = other.m_watchableItem;
  m_watchedDescriptor = other.m_watchedDescriptor;
  return *this;
}

std::string WatchedItem::path() const
{
  return m_watchableItem.path();
}

bool WatchedItem::handleEvent(WatchedEvent event)
{
  if (event.watchedDescriptor == m_watchedDescriptor)
  {
    return m_watchableItem.handleEvent(event);
  }
  else if (m_watchableItem.match(m_watcher, event))
  {
    return m_watchableItem.handleEvent(event);
  }
  return false;
}

bool WatchedItem::handleRelatedEvent(WatchedEvent event)
{
  return m_watchableItem.handleRelatedEvent(m_watcher, event);
}

WatchableItem const &WatchedItem::watchableItem() const
{
  return m_watchableItem;
}

int WatchedItem::descriptor() const
{
  return m_watchedDescriptor;
}

Watcher::Watcher() : m_inotifyFileDescriptor(-1), m_keepRunning(true), m_maximumWatchedFileDescriptors(DEFAULT_MAXIMUM_WATCHED_FILE_DESCRIPTORS), m_testmode(0), m_sync(false)
{
  _open_inotify();
}

Watcher::~Watcher()
{
  if (watching())
  {
    close(m_inotifyFileDescriptor);
  }
}

void Watcher::setMaximumWatchedFileDescriptors(size_t n)
{
  m_maximumWatchedFileDescriptors = n;
}

bool Watcher::watching() const
{
  return m_inotifyFileDescriptor >= 0;
}

void Watcher::start(WatchList const watchList, bool sync, int testMode)
{
  m_sync = sync;
  m_testmode = testMode;
  std::copy(watchList.begin(), watchList.end(), std::back_inserter(m_watchList));
  if ( sync ) {
    _sync();
  }
  _watchItems();
  _matchItems();
  _process_inotify_events();
}

void Watcher::_sync() {
  // first part of sync which ensures that all source files and directories are 
  // accounted for in target
  std::cout << "SYNCING FILE SYSTEM TARGETS" << std::endl;
  // Here we are going to ensure that all files from a source are the same.
  // If there are no sources we cannot sync.
  bool hasAtLeastOneSource = false;
  for( auto w : m_watchList ) {
    if ( w.source() ) {
      hasAtLeastOneSource = true;
      break;
    }
  }
  if ( !hasAtLeastOneSource ) {
    std::cerr << "No source was found, no sync possible as there is not a version which is selected as 'the truth'. Use --source to mark a watched file system object as a source." << std::endl;
    exit(EXIT_FAILURE);
  }
  // We do have a source so we can now ensure that all items in that source appear in the target and have the same contents (if a file).
  // For directories in the source, we ensure there are no additional files in the target. If there are then we warn but we don't stop as this is a valid situation.
  std::vector<File> srcFileList;
  std::vector<std::string> srcRoots;
  std::vector<File> rootTargetFileList;
  std::vector<File> existingTargetFileList;
  std::vector<std::string> existingTargetRoots;
  for( auto w : m_watchList ) {
    std::string wAbsPath = w.abspath();
    File f(wAbsPath);
    if ( w.source() ) {
      f.crawl();
      srcFileList = f.files();
      for( size_t ii = 0; ii < srcFileList.size(); ii++ ) {
        srcRoots.push_back(wAbsPath);
      }
    } else {
      f.crawl();
      existingTargetFileList = f.files();
      for( size_t ii = 0; ii < existingTargetFileList.size(); ii++ ) {
        existingTargetRoots.push_back(wAbsPath);
      }
      rootTargetFileList.push_back(f);
    }
  }

  // check that all files that are in the source, are in the destination.
  size_t ii=0; 
  for ( auto& s : srcFileList ) {
    std::string remainder = s.abspath().substr(srcRoots[ii].size());
    if ( remainder.size() > 0 ) {
      for ( auto& t : rootTargetFileList ) {
        if ( t.isDirectory() == true ) {
          File maybeFileLocation(t.abspath() + remainder);
          if ( !maybeFileLocation.exists() ) {
            std::cout << "File/Directory does not exist: " << maybeFileLocation.path() << std::endl;
            maybeFileLocation.copyFrom(s);
          }
        }
      }
    }
  }
}

void Watcher::_matchItems() {
  for ( auto& w : m_watchedList ) {
    std::string p = w.watchableItem().abspath();
    std::string root = w.watchableItem().root();
    std::string pathWithoutRoot = p.substr(root.size()); // the +1 is for the final separator

    for( auto& w2 : m_watchedList) {
      if ( w2.watchableItem().source() == false ) {
        if ( w2.watchableItem().abspath() != p ) {
          std::string p2 = w2.watchableItem().abspath();
          std::string root2 = w2.watchableItem().root();
          if ( root2.size() <= p2.size() ) {
            std::string pathWithoutRoot2 = p2.substr(root2.size()); // the +1 is for the final separator
            if (  pathWithoutRoot == pathWithoutRoot2 ) {
              std::cout << "Setting " << p2 << " as a copy target of " << p << std::endl;
              m_copyTargets.insert(std::make_pair(w.descriptor(), std::reference_wrapper(w2)));
              if ( m_sync ) {
                std::cout << "Syncing with source" << std::endl;
                File fp2(p2);
                File fp(p);
                fp2.copyFrom(fp);
              }
            }
          }
        }
      }
    }
  }
}

void Watcher::stop()
{
  m_keepRunning = false;
}

WatchableItem const &Watcher::getWatchableItem(int descriptor) const
{
  for (auto &w : m_watchedList)
  {
    if (w.descriptor() == descriptor)
    {
      return w.watchableItem();
    }
  }
  throw std::runtime_error("Watchable item with descriptor not found.");
}

void Watcher::_open_inotify()
{
  m_inotifyFileDescriptor = inotify_init();
  if (m_inotifyFileDescriptor < 0)
  {
    std::cerr << "inotify_init () = " << m_inotifyFileDescriptor << std::endl;
  }
}

void Watcher::_process_inotify_events()
{
  size_t loopCounter = 0;
  if ( m_testmode == 1 && loopCounter == 0 ) {
    std::cout << "Test mode 1 - breaking process loop after " << loopCounter << " loops." << std::endl;
    return;
  }
  while (m_keepRunning && (m_watchedList.size() > 0))
  {
    if (_event_check() > 0)
    {
      int inBytes = _read_events();
      if (inBytes < 0)
      {
        break;
      }
      else
      {
        _handle_events();
      }
    }
    loopCounter++;
  }
}

int Watcher::_event_check()
{
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(m_inotifyFileDescriptor, &rfds);
  /* Wait until an event happens or we get interrupted
    by a signal that we catch */
  return select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
}

int Watcher::_read_events()
{
  char buffer[16384];
  size_t bufferIndex;
  struct inotify_event *pevent;
  WatchedEvent event;
  int count = 0;

  ssize_t inBytes = read(m_inotifyFileDescriptor, buffer, 16384);
  if (inBytes <= 0)
    return inBytes;
  bufferIndex = 0;
  while (bufferIndex < inBytes)
  {
    /* Parse events and queue them. */
    pevent = (struct inotify_event *)&buffer[bufferIndex];
    event.watchedDescriptor = pevent->wd;
    event.mask = pevent->mask;
    event.cookie = pevent->cookie;
    event.name.assign(pevent->name, pevent->len);

    if (event.mask & IN_ISDIR)
    {
      event.type = WATCH_DIRECTORY;
    }
    else
    {
      event.type = WATCH_FILE;
    }

    event.flags = event.mask & ~(IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW | IN_IGNORED);
    event.eventType = event.mask & (IN_ALL_EVENTS | IN_UNMOUNT | IN_Q_OVERFLOW | IN_IGNORED);

    if (event.flags & (~IN_ISDIR))
    {
      std::cout << "Other flags detected in event=" << event.flags << std::endl;
      ;
    }
    m_eventQueue.push(event);

    bufferIndex += sizeof(struct inotify_event) + pevent->len;
    count++;
  }
  return count;
}

void Watcher::_handle_events()
{
  while (!m_eventQueue.empty())
  {
    auto qItem = m_eventQueue.front();
    m_eventQueue.pop();
    for (auto &wItem : m_watchedList)
    {
      wItem.handleEvent(qItem);
    }

    auto copyTargets = m_copyTargets.equal_range(qItem.watchedDescriptor);
    for( auto copyTarget = copyTargets.first; copyTarget != copyTargets.second; ++copyTarget) {
      auto destination = copyTarget->second;
      destination.get().handleRelatedEvent(qItem);
    }
  }
}

void Watcher::_watchItem(WatchableItem item)
{
  int wd = inotify_add_watch(m_inotifyFileDescriptor, item.path().c_str(), item.mask());
  if (wd < 0)
  {
    std::cerr << "Cannot add watch for \"" << item.path() << "\" with event mask 0x" << std::hex << std::setfill('0') << std::setw(8) << item.mask() << std::endl;
    fflush(stdout);
    perror(" ");
  }
  else
  {
    WatchedItem watchedItem(*this, item, wd);
    m_watchedList.push_back(watchedItem);
    std::cout << "Watching " << item.abspath() << std::endl;
  }
}

void Watcher::_watchItems()
{
  for (auto &item : m_watchList)
  {
    // if item is a directory we need to want to watch all the children
    // if item is a file then we just want to watch that one file.
    File watchFile(item.path());
    if (isTrue(watchFile.isDirectory()))
    {
      // crawl will include the top directory as the last item
      // it does it in a depth first search so lowests level
      // this minimises access events.
      watchFile.crawl();
      for (auto & f : watchFile.files())
      {
        auto w = WatchableItem(f.path(), item.abspath());
        w.setReadonly(item.readonly());
        w.setSource(item.source());
        _watchItem(w);

        if (m_watchedList.size() >= m_maximumWatchedFileDescriptors)
        {
          std::cout << "[WARNING] Maximum watched files reached (" << m_watchList.size() << ")" << std::endl;
          return;
        }
      }
    }
    else
    {
      // if we do this before we will get a whole load of events which we are not interested in
      item.setRoot(item.abspath());
      _watchItem(item);
    }
    if (m_watchedList.size() >= m_maximumWatchedFileDescriptors)
    {
      std::cout << "[WARNING] Maximum watched files reached (" << m_watchList.size() << ")" << std::endl;
      return;
    }
  }
  std::cout << "Watching " << m_watchedList.size() << " items" << std::endl;
}
