#ifndef INOTIFY_H
#define INOTIFY_H

#include <map>
#include <string>
#include <cstdint>

std::map<uint32_t, std::string> inotifyEvents = {
  { IN_OPEN, "OPEN" },
  { IN_CLOSE, "CLOSE" },
  { IN_ACCESS, "ACCESS" },
  { IN_MODIFY, "MODIFY" },
  { IN_ATTRIB, "ATTRIB" },
  { IN_CLOSE_WRITE, "CLOSE_WRITE" },
  { IN_CLOSE_NOWRITE, "CLOSE_NOWRITE" },
  { IN_MOVE_SELF, "MOVE_SELF" },
  { IN_MOVED_FROM, "MOVED_FROM" },
  { IN_MOVED_TO, "MOVED_TO" },
  { IN_IGNORED, "IGNORED" },
  { IN_DELETE, "DELETE" },
  { IN_DELETE_SELF, "DELETE_SELF" },
  { IN_CREATE, "CREATE" },
};

#endif /* INOTIFY_H */
