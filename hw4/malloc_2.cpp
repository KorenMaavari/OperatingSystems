#include <stdio.h> // for size_t
#include <cmath> // for pow
#include <unistd.h> // for sbrk
#include <cstring> // for memmove and memset

// Metadata structure
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

// Global pointer to the list of metadata structures
MallocMetadata* metadata_head = nullptr;

// Function to append metadata to the list
void append_metadata (MallocMetadata* new_metadata) {
    if (!metadata_head) {
        metadata_head = new_metadata;
    } else {
        MallocMetadata* current = metadata_head;
        while (current->next) {
            current = current->next;
        }
        current->next = new_metadata;
        new_metadata->next = NULL;
        new_metadata->prev = current;
    }
}

// Function to allocate memory
void* smalloc(size_t size) {
    // Check for invalid size
    if (size == 0 || size > pow(10, 8)) {
        return NULL;
    }
    // Search for a free block
    MallocMetadata* current = metadata_head;
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = false;
            return (char*)current + sizeof(MallocMetadata); // Skip metadata
        }
        current = current->next;
    }
    // Allocate new block if no free block is found
    void* result = sbrk(size + sizeof(MallocMetadata));
    if (result == (void*)(-1)) {
        return NULL;
    }
    MallocMetadata* new_metadata = (MallocMetadata*)result;
    new_metadata->size = size;
    new_metadata->is_free = false;
    new_metadata->next = nullptr;
    new_metadata->prev = nullptr;
    append_metadata(new_metadata);
    return (char*)result + sizeof(MallocMetadata); // Skip metadata
}

// Function to allocate memory and initialize to zero
void* scalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = smalloc(total_size);
    if (ptr) {
        std::memset(ptr, 0, total_size);
    }
    return ptr;
}

void sfree(void* p) {
    if (!p) {
        return;
    }
    MallocMetadata* metadata = (MallocMetadata*)((char*)p-sizeof(MallocMetadata));
    metadata->is_free = true;
}

// Function to reallocate memory
void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > pow(10, 8)) {
        return nullptr; // Invalid size, return nullptr
    }
    if (!oldp) {
        // If oldp is nullptr, perform allocation
        return smalloc(size);
    }
    // Get pointer to metadata of old block
    MallocMetadata* old_metadata = (MallocMetadata*)((char*)oldp-sizeof(MallocMetadata));
    if (old_metadata->size >= size) {
        // If old block size is sufficient, return old pointer
        return oldp;
    }
    // Allocate new memory block
    void* newp = smalloc(size);
    if (!newp) {
        return nullptr; // Allocation failed, return nullptr
    }
    // Copy data from old block to new block
    std::memmove(newp, oldp, old_metadata->size);
    // Free old memory block
    sfree(oldp);
    return newp;
}

// Functions to retrieve memory statistics

// Returns the number of allocated blocks that are currently free
size_t _num_free_blocks() {
    size_t count = 0;
    MallocMetadata* current = metadata_head;
    while (current) {
        if (current->is_free) {
            count++;
        }
        current = current->next;
    }
    return count;
}

// Returns the number of bytes in all allocated blocks that are currently free
size_t _num_free_bytes() {
    size_t count = 0;
    MallocMetadata* current = metadata_head;
    while (current) {
        if (current->is_free) {
            count += current->size;
        }
        current = current->next;
    }
    return count;
}

// Returns the overall number of allocated blocks (both free and in use)
size_t _num_allocated_blocks() {
    size_t count = 0;
    MallocMetadata* current = metadata_head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// Returns the overall number of allocated bytes (both free and in use)
size_t _num_allocated_bytes() {
    size_t count = 0;
    MallocMetadata* current = metadata_head;
    while (current) {
        count += current->size;
        current = current->next;
    }
    return count;
}

// Returns the overall number of metadata bytes currently in use
size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

// Returns the size of a single metadata structure
size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
