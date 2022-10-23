# INotify Demo

Playing about with the inotify protocol for watching files

## Getting Started
### Building

```
make

# use clean to delete all temporary files
make clean 

# create a test directory of stuff to use for tests
./create_test_directory.sh
```

You can then copy the watcher binary to the directory of your choice.

### Usage

```
./watcher [--sync] [--source <source tree>] <target tree 1> [<target tree 2> ...] 
```

* Use --sync to ensure that all files and directories in the source tree and in the target trees. It will also ensure that the contents of all files in the source tree is copied over to the target tree.  Any files in the target tree, not in the source tree will not be touched.
* Use --source to mark one of the tree as the unconditional truth - this is common if this is the one you are editing.
* When one of the files changes in the source tree then the change will propagate to all target trees.
* If there is NO source tree, then any changes in one target tree will propagate to all other target trees.

## Resources

* [https://developer.ibm.com/tutorials/l-inotify/](https://developer.ibm.com/tutorials/l-inotify/)

