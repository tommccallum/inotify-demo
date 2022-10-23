#ifndef WATCHED_EVENT_H
#define WATCHED_EVENT_H

#include <cstdint>
#include <string>

enum WatchedType {
  WATCH_FILE = 0,
  WATCH_DIRECTORY
};


struct WatchedEvent {
  int watchedDescriptor;
  uint32_t mask;
  uint32_t cookie;
  std::string name;
  WatchedType type;
  uint32_t flags;
  uint32_t eventType;
};


#endif /* WATCHED_EVENT_H */
