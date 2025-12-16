#include "SharedMemory.hpp"

SharedMemory::SharedMemory(bool create) : fd(-1), _root(nullptr), owner(create) {
    if (create) {
        shm_unlink(SHM_NAME);
        
        fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            throw std::runtime_error("Failed to create shared memory");
        }
        
        if (ftruncate(fd, sizeof(SharedMemoryRoot)) == -1) {
            close(fd);
            shm_unlink(SHM_NAME);
            throw std::runtime_error("Failed to set size of shared memory");
        }
    } else {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd == -1) {
            throw std::runtime_error("Failed to open shared memory. Is server running?");
        }
    }
    
    _root = static_cast<SharedMemoryRoot*>(
        mmap(nullptr, sizeof(SharedMemoryRoot), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)
    );
    
    if (_root == MAP_FAILED) {
        close(fd);
        if (create) shm_unlink(SHM_NAME);
        throw std::runtime_error("Failed to map shared memory");
    }
    
    if (create) {
        memset(_root, 0, sizeof(SharedMemoryRoot));
        
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&_root->mutex, &mattr);
        pthread_mutexattr_destroy(&mattr);
        
        pthread_condattr_t cattr;
        pthread_condattr_init(&cattr);
        pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&_root->server_cond, &cattr);
        
        for (size_t i = 0; i < MAX_CLIENTS; i++) {
            pthread_cond_init(&_root->clients[i].cond, &cattr);
            _root->clients[i].current_game_id = -1;
        }
        
        pthread_condattr_destroy(&cattr);
        
        _root->q_head = 0;
        _root->q_tail = 0;
        _root->game_count = 0;
    }
}

SharedMemory::~SharedMemory() {
    if (_root && _root != MAP_FAILED) {
        munmap(_root, sizeof(SharedMemoryRoot));
    }
    
    if (fd != -1) {
        close(fd);
    }
    
    if (owner) {
        shm_unlink(SHM_NAME);
    }
}
