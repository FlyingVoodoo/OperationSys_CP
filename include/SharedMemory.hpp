#pragma once
#include "SharedTypes.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class SharedMemory {
public:
    SharedMemory(bool create = false);
    ~SharedMemory();

    SharedMemoryRoot* root() { return _root; }
    bool is_owner() const { return owner; }

private:
    int fd;
    SharedMemoryRoot* _root;
    bool owner;
};
