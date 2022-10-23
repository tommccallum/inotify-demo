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
#include "Watcher.h"
#include "File.h"

void signal_handler(int signum);
int main(int argc, char **argv);

using namespace std;

Watcher watcher;

/* Signal handler that simply resets a flag to cause termination */
void signal_handler(int signum)
{
  watcher.stop();
}

int main(int argc, char **argv)
{
  WatchList watchList;
  
  if ( !watcher.watching() ) {
    std::cerr << "Failed to open inotify handle." << std::endl;
    exit(EXIT_FAILURE);
  }

  // read in directories and files to watch from command line
  int argii=1;
  int sourceCount = 0;
  bool sync = false;
  bool color = true;
  int testMode = 0;
  while( argii < argc ) {
    if ( argv[argii][0] == '-' ) {
      if ( argv[argii][1] == '-' ) {
        // long option
        if ( strcmp(argv[argii], "--source" ) == 0) {
          if ( sourceCount > 0 ) {
            std::cerr << "Only allowed one source of truth." << std::endl;
            exit(EXIT_FAILURE);
          }
          sourceCount++;
          // If a directory is a source then changes are only ever coped from 
          // this directory and never to it.
          if ( argc > argii+1 ) {
            watchList.emplace_back(argv[argii+1]);
            watchList.back().setReadonly(true);
            watchList.back().setSource(true);
            argii+=2;
          } else {
            std::cerr << "No source directory for argument --source." << std::endl;
            exit(EXIT_FAILURE);
          }
        } else if ( strcmp(argv[argii], "--sync") == 0 ) {
          std::cout << "Enabling target synchronisation" << std::endl;
          sync = true;
          argii+=1;
        } else if ( strcmp(argv[argii], "--test-mode") == 0 ) {
          testMode = std::atoi(argv[argii+1]);
          if ( testMode  > 0 ) {
            std::cout << "Test mode " << testMode << " enabled" << std::endl;
          }
          argii+=2;
        } else {
          std::cerr << "Unexpected argument " << argv[argii] << std::endl;
          exit(EXIT_FAILURE);
        }
      } else {
        // short option
        if ( argv[argii][1] == 'u' ) {
          int maximumWatchedFileDesciptors = std::atoi(argv[argii+1]);
          watcher.setMaximumWatchedFileDescriptors(maximumWatchedFileDesciptors);
          argii += 2;
        } else {
          std::cerr << "Unexpected argument " << argv[argii] << std::endl;
          exit(EXIT_FAILURE);
        }
      }
    } else {
      File f(argv[argii]);
      
      // std::cout << "Basename: " << f.basename() << std::endl;
      // std::cout << "Dirname: " << f.dirname() << std::endl;
      // std::cout << "Abspath: " << f.abspath() << std::endl;

      watchList.emplace_back( argv[argii] );
      argii++;
    }
  }

  // Remove any paths which are subpaths of others.
  // if the readonly subpath but the parent path is not readonly then 
  // we should error.
  std::set<size_t> removeIndices;
  WatchList filteredWatchList;
  for( size_t ii =0; ii < watchList.size(); ii++ ) {
    auto& p1 = watchList[ii];

    if ( removeIndices.find(ii) == removeIndices.end() ) {
      File f1(p1.abspath());
      for(size_t jj =0; jj < watchList.size(); jj++ ) {
        if ( ii != jj ) {
          auto& p2 = watchList[jj];
          File f2(p2.abspath());
          if ( f2.isSubpathOf(f1) ) {
            std::cout << f2.abspath() << " is subpath of " << f1.abspath()  << std::endl;
            if ( p2.source() && !p1.source() ) {
              std::cerr << "source path " << p2.abspath() << " is a subpath of non-source path " << p1.abspath() << ". This is not allowed." << std::endl;
              exit(EXIT_FAILURE);
            } else {
              removeIndices.insert(jj);
              break;
            }
          }
        }
      }
    }
  }
  for( size_t ii=0; ii < watchList.size(); ii++ ) {
    if ( removeIndices.find(ii) == removeIndices.end() ) {
      filteredWatchList.push_back(watchList[ii]);
    } else {
      std::cout << "[NOTICE] Removing duplicate path: " << watchList[ii].path() << std::endl;
    }
  }
  
  if ( watchList.empty() ) {
    exit(EXIT_SUCCESS);
  }

  
  /* Set a ctrl-c signal handler */
  if (signal(SIGINT, signal_handler) == SIG_IGN)
  {
    /* Reset to SIG_IGN (ignore) if that was the prior state */
    signal(SIGINT, SIG_IGN);
  }

  watcher.start(watchList, sync, testMode);

  std::cout << "\nTerminating\n";
  return 0;
}
