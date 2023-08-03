# Virtual file system MyVFS

The main principle of the system is to decrease the count of system calls and hard drive accesses as much as possible.

Another principle is *end-to-end argument*: all elements of the system are implemented from scratch with the usage of low-level primitives. Here is why it could be useful:

1. Platform independency - virtual file system (FS) is independent of certain platform implementations and only bound to C++ standard
2. Transparency - virtual FS gives us some guarantees, which we can rely on during code writing (or, vice versa, we can change virtual FS code, if necessary)
3. Efficiency - we pay only for what we need, and we can change the system if necessary (see "Transparency")

The system consists of 2 main parts - a RAM cache system and a "file manager", class that directly communicates with a hard drive.

**Cache**

There is RAM LRU-Cache in the system, which is an addition to the cache, which is implemented as usual in an OS and used while working with the hard drive.

The cache system is configurable: we can change the cache limit size, or change the "cache clearing percentage" (after reaching the maximum cache size, a separate worker thread will delete old data up to the moment when no more than X% of the cache capacity will be occupied)

**FileManager**

FileManager is a superstructure over the worker thread that directly communicates with a hard drive.

For the optimizations of read/write system calls (and therefore hard drive accesses), the SegmentSystem class has been written. This class represents a segment system, with the possibility of addition and deletion of different segments. With this class, we can:

* Track the free space in the physical file to make virtual file writes in this free space
* Optimize hard drive accesses with the fusion of the data segments that are located close to each other. I.e., if we try to read file X that occupies a segment with index i<sub>X</sub> and read file Y that occupies i<sub>X+1</sub>, then both reads would happen with the single read access to the hard drive. The same goes for writing. Therefore, if virtual file segments are located "in a suitable manner" in physical files, the system call costs will remain low while the reads/writes are increasing.

The FileManager is implemented with a lock-free Michael-Scott queue to serialize read/write file tasks and executes these tasks while working on a warm physical cache like the strand synchronization primitive does.

**Concurrency**

There are some synchronization primitives implemented for this virtual FS.

- Read-Write Ticket Lock - ticket lock which implements reader-writer pattern. This is implemented instead of std::shared_mutex for the reason that it gives the FIFO guarantees to guarantee the starvation absence (std::shared_mutex does not give such guarantees, at least open documentation does not state this)

- MSQueue, or Michael-Scott queue, the lock-free queue. Memory management is provided with the garbage collection by the separate class "Journal"

**Possible cons**

The system is expected to work the most efficiently when the number of available file descriptors =~ worker threads optimal count. If available file descriptors amount >> worker threads optimal count, perhaps the next implementation could be better:

With the usage of std::shared_mutex, we will pick the correspondent lock during a file write/read, and we will work with a correspondent physical file, after mapping a virtual file to it (for instance, using the same round-robin algorithm) This implementation may be more efficient also because almost no auxiliary classes are required.

**Async**

Also, the system works better if we would use async file reads/writes.

**Testing**

All tests are located in the Tests/ folder, the system could be built and tested with `make test`