#include <stdio.h> // for size_t
#include <cmath> // for pow
#include <unistd.h> // for sbrk
#include <cstring> // for memmove and memset
#include <sys/mman.h> // for mmap and munmap

// Metadata structure
const int MAX_ORDER = 10;
const size_t MAX_SIZE = 128 * 1024;
const int BLOCKS_NUM = 32;
const size_t MAX_SCALLOC = 1 <<21;
const size_t MAX_SMALLOC = MAX_SCALLOC*2;

struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    int block_index;
    int order;
    bool org_scalloc;
    size_t scalloc_size;
};

// Class for Buddy Allocator
class BuddyAllocator {
    private:
        bool init;
        size_t size_by_order[MAX_ORDER + 1];
        MallocMetadata* free_blocks[MAX_ORDER + 1];
        void appendToFreeData(MallocMetadata* mallocMetadata, int order);
        void removeFromFreeData(MallocMetadata* mallocMetadata, int order);
        int findOrder(size_t size);
        void splitBlock(MallocMetadata* block, int order);
        MallocMetadata* mergeBlocks(MallocMetadata* block, int order);
        MallocMetadata* mergeBlocksSrealloc(MallocMetadata* block, int order, size_t wanted_size);
        bool checkMergeBlocks(MallocMetadata *block, int block_order, MallocMetadata *buddy,
                              size_t wanted_size);
        void insert_to_allocated (MallocMetadata* to_add);
        void delete_from_allocated (MallocMetadata* mallocMetadata);
        void init_blocks();
        size_t find_huge_page_size(size_t size);
        MallocMetadata* tail_of_allocated;
    public:
        BuddyAllocator();
        void* smalloc(size_t size, bool from_scaloc, size_t size_block);
        void sfree(void* p);
        void* srealloc(void* oldp, size_t size);
        void* scalloc(size_t num, size_t size);
        size_t _num_free_blocks();
        size_t _num_free_bytes();
        size_t _num_allocated_blocks();
        size_t _num_allocated_bytes();
        size_t _num_meta_data_bytes();
        size_t _size_meta_data();

};

// Initialize free_blocks and allocate initial memory blocks
BuddyAllocator::BuddyAllocator() : init(true), tail_of_allocated(nullptr)  {
    size_t curr_size = 128;
    for (int i = 0; i < MAX_ORDER + 1; ++i) {
        free_blocks[i] = nullptr;
        size_by_order[i] = curr_size;
        curr_size*=2;
    }
}

// Append metadata to the list of free blocks
void BuddyAllocator::appendToFreeData(MallocMetadata* mallocMetadata, int order) {
    if (!free_blocks[order]) {
        free_blocks[order] = mallocMetadata;
        mallocMetadata->next = nullptr;
        mallocMetadata->prev = nullptr;
    }
    else {
        MallocMetadata* current = free_blocks[order];
        if(mallocMetadata < current){
            // add as first element
            mallocMetadata->next = current;
            mallocMetadata->prev = nullptr;
            current->prev = mallocMetadata;
            free_blocks[order] = mallocMetadata;
            return;
        }
        while(current){
            // current - the element before mallocMetadata in the insert
            if(!current->next){
                // add to the end of the list
                mallocMetadata->next = nullptr;
                mallocMetadata->prev = current;
                current->next = mallocMetadata;
                return;
            }
            if(mallocMetadata <= current->next){
                // middle insert
                MallocMetadata* next = current->next;
                current->next = mallocMetadata;
                mallocMetadata->next = next;
                mallocMetadata->prev = current;
                next->prev = mallocMetadata;
                return;
            }
            current = current->next;
        }


    }
}

// Find the order of a given metadata structure
int BuddyAllocator::findOrder(size_t size) {
    size_t current_size = MAX_SIZE;
    int order = MAX_ORDER;
    while (size <= current_size/2 && order>0) {
        current_size /= 2;
        order--;
    }
    return order;
}

void BuddyAllocator::removeFromFreeData(MallocMetadata *mallocMetadata, int order)
{
    MallocMetadata *toDelete = free_blocks[order];
    while (toDelete){
        if(toDelete == mallocMetadata){
            break;
        }
        else {
            toDelete = toDelete->next;
        }
    }
    if(!toDelete){
        return;
    }

    MallocMetadata* tail = free_blocks[order]; // tail 1->2->3 head
    MallocMetadata* head = tail;
    while (head->next){
        head = head->next;
    }

    if(!toDelete->next && !toDelete->prev){
        free_blocks[order] = nullptr;
    }

    else if(toDelete == tail)
    {
        tail->next->prev = nullptr;
        free_blocks[order] = tail->next;
    }
    else if(toDelete == head){
        head->prev->next = nullptr;
    }
    else {
        toDelete->prev->next = toDelete->next;
        toDelete->next->prev = toDelete->prev;
    }
}

// Function to allocate memory
void* BuddyAllocator::smalloc(size_t size, bool from_scaloc, size_t size_block) {
    if(init){
        init_blocks();
        init = false;
    }
    // Check for invalid size
    if (size == 0 || size > pow(10, 8)) {
        return NULL;
    }
    // Check if large allocation
    if (size + sizeof(MallocMetadata) > MAX_SIZE) {
        void* block;
        if(size >= MAX_SMALLOC && !from_scaloc){
            block = mmap(nullptr,find_huge_page_size(size + sizeof(MallocMetadata)),PROT_READ | PROT_WRITE,
                                          MAP_HUGETLB | MAP_ANONYMOUS| MAP_PRIVATE,-1, 0);
        }
        else if(size_block >= MAX_SCALLOC && from_scaloc){
            block = mmap(nullptr,find_huge_page_size(size + sizeof(MallocMetadata)),PROT_READ | PROT_WRITE,
                                          MAP_HUGETLB | MAP_ANONYMOUS| MAP_PRIVATE,-1, 0);
        }
        else {
            block = mmap(nullptr, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        }
        if(block == (void*)-1)
        {
            return nullptr;
        }
        // Create metadata
        MallocMetadata* metadata = (MallocMetadata*)(block);
        metadata->size = size;
        metadata->is_free = false;
        metadata->next = nullptr;
        metadata->prev = nullptr;
        metadata->block_index = -1;
        metadata->order = -1;
        metadata->org_scalloc = from_scaloc;
        metadata->scalloc_size = size_block;
        insert_to_allocated(metadata);
        return (char*)metadata + sizeof(MallocMetadata);
    }
    int order = findOrder(size);
    size_t block_size = size_by_order[order];
    // Search for a free block
    while (order < MAX_ORDER + 1) {
        if (free_blocks[order]) {
            MallocMetadata* block = free_blocks[order];
            while (block_size / 2 >= size + sizeof(MallocMetadata) && order>0) {
                splitBlock(block, order);
                order--;
                block_size /= 2;
            }
            block->is_free = false;
            block->size = size;
            block->org_scalloc = from_scaloc;
            removeFromFreeData(block, block->order);
            insert_to_allocated(block);
            return (char*)block + sizeof(MallocMetadata);
        }
        order++;
        block_size*=2;
    }
    return nullptr;
}

void BuddyAllocator::sfree(void* p) {
    if (!p) {
        return;
    }
    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    if(metadata->is_free){
        return;
    }
    if (metadata->order == -1) {
        metadata->is_free = true;
        delete_from_allocated(metadata);
        if(metadata->scalloc_size >=MAX_SCALLOC && metadata->org_scalloc){
            munmap(metadata, find_huge_page_size(metadata->size + sizeof(MallocMetadata)));
            return;

        }
        if(metadata->size >=MAX_SMALLOC && !metadata->org_scalloc){
            munmap(metadata, find_huge_page_size(metadata->size + sizeof(MallocMetadata)));
            return;

        }
        munmap(metadata, metadata->size + sizeof(MallocMetadata));
        return;
    }
    metadata->is_free = true;
    delete_from_allocated(metadata);
    appendToFreeData(metadata, metadata->order);
    mergeBlocks(metadata, metadata->order);
}

// Split a block into two buddies
void BuddyAllocator::splitBlock(MallocMetadata* block, int order) {
    //size_t block_size = pow(2, order); // TODO: Ask what the difference between 128bytes block and 2^order block is.
    size_t block_size = size_by_order[order]/2;
    // Create metadata for the new buddy block
    void* buddyAddress = (void*)((unsigned long)block ^ block_size);
    MallocMetadata* buddy = (MallocMetadata*)(buddyAddress);
    buddy->size = block_size;
    buddy->is_free = true;
    buddy->next = nullptr;
    buddy->prev = nullptr;
    buddy->block_index = block->block_index;
    block->order -= 1;
    block->size = block_size;
    buddy->order = block->order;
    // Remove original block from its original free list
    removeFromFreeData(block, order);
    // Add both the new block and the buddy to the appropriate free list
    appendToFreeData(block, order - 1);
    appendToFreeData(buddy, order - 1);
}

// Merge two buddy blocks
MallocMetadata* BuddyAllocator::mergeBlocks(MallocMetadata* block, int order) {
    //size_t block_size = pow(2, order);
    size_t block_size = size_by_order[order];
    // Calculate address of the buddy block
    void* buddyAddress = (void*)((unsigned long)block ^ block_size);
    MallocMetadata* buddy = (MallocMetadata*)(buddyAddress);
    if (buddy->is_free && buddy->order == block->order && buddy->block_index == block->block_index
    && block_size != MAX_SIZE) {
        removeFromFreeData(block, block->order);
        removeFromFreeData(buddy, buddy->order);
        // Update size and order of merged block
        if(buddy < block) {
            block = buddy;
        }
        block->order += 1;
        // Recursively merge if possible
        appendToFreeData(block, block->order);
        return mergeBlocks(block, order + 1);
    }
    return block;
}


// Function to reallocate memory
void* BuddyAllocator::srealloc(void* oldp, size_t size) {
    if (size == 0 || size > pow(10, 8)) {
        return nullptr; // Invalid size, return nullptr
    }
    if (!oldp) {
        // If oldp is nullptr, perform allocation
        return smalloc(size, false, size);
    }
    // Get pointer to metadata of old block
    MallocMetadata* old_metadata = (MallocMetadata*)((char*)oldp-sizeof(MallocMetadata));

    if(size == old_metadata->size){
        return oldp;
    }

    if(old_metadata->size + sizeof(MallocMetadata) <= MAX_SIZE) {
        // no use of mmap
        if (size_by_order[old_metadata->order] >= size) {
            // If old block size is sufficient, return old pointer
            old_metadata->size = size;
            return oldp;
        }
        void* buddyAddress = (void*)((unsigned long)old_metadata ^ size_by_order[old_metadata->order]);
        MallocMetadata* buddy = (MallocMetadata*)(buddyAddress);
        if(checkMergeBlocks(old_metadata, old_metadata->order, buddy,size)){
            delete_from_allocated(old_metadata);
            old_metadata->is_free = true;
            appendToFreeData(old_metadata, old_metadata->order);
            MallocMetadata* merged = mergeBlocksSrealloc(old_metadata, old_metadata->order, size);
            merged->size = size;
            merged->is_free= false;
            removeFromFreeData(merged, merged->order);
            insert_to_allocated(merged);
            return (char*)merged + sizeof(MallocMetadata);
        }
    }
    // Allocate new memory block
    void* newp = smalloc(size, old_metadata->org_scalloc, size);
    if (!newp) {
        return nullptr; // Allocation failed, return nullptr
    }
    // Copy data from old block to new block
    std::memmove(newp, (char*)oldp + sizeof(MallocMetadata), old_metadata->size);
    // Free old memory block
    sfree(oldp);
    return newp;
}

bool BuddyAllocator::checkMergeBlocks(MallocMetadata *block, int block_order, MallocMetadata *buddy,
                                      size_t wanted_size) {
    // we do not want to change the actual block / buddy
    size_t block_size = size_by_order[block_order];
    // Calculate address of the buddy block
    if (buddy->is_free && buddy->order == block_order && buddy->block_index == block->block_index
        && block_size != MAX_SIZE && 2* block_size - sizeof(MallocMetadata)< wanted_size) {
        block_size *= 2;
        if(buddy < block){
            block = buddy;
        }
        void* buddyAddress = (void*)((unsigned long)block ^ block_size);
        buddy = (MallocMetadata*)(buddyAddress);
        block_order++;
        return checkMergeBlocks(block, block_order, buddy, wanted_size);
    }
    else if(buddy->is_free && buddy->order == block_order && buddy->block_index == block->block_index
       && block_size != MAX_SIZE && block_size *2 - sizeof(MallocMetadata) >= wanted_size){
        return true;
    }

    return false;
}

// Function to allocate memory and initialize to zero
void* BuddyAllocator::scalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void* ptr = smalloc(total_size, true, size);
    if (ptr) {
        std::memset(ptr, 0, total_size);
    }
    return ptr;
}

// Returns the number of allocated blocks that are currently free
size_t BuddyAllocator::_num_free_blocks() {
    size_t count = 0;
    for (int i=0 ; i<=MAX_ORDER ; i++) {
        MallocMetadata* current = free_blocks[i];
        while (current) {
            count++;
            current = current->next;
        }
    }
    return count;
}

// Returns the number of bytes in all allocated blocks that are currently free
size_t BuddyAllocator::_num_free_bytes() {
    size_t count = 0;
    for (int i=0 ; i<=MAX_ORDER ; i++) {
        MallocMetadata* current = free_blocks[i];
        while (current) {
            count += size_by_order[current->order] - sizeof(MallocMetadata);
            current = current->next;
        }
    }
    return count;
}

// Returns the overall number of allocated blocks (both free and in use)
size_t BuddyAllocator::_num_allocated_blocks() {
    size_t count = _num_free_blocks();
    // count += allocated
    MallocMetadata* current = tail_of_allocated;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

size_t BuddyAllocator::_num_allocated_bytes() {
    size_t count = _num_free_bytes();
    // count += allocated
    MallocMetadata* current = tail_of_allocated;
    while (current) {
        if(current->order>=0) {
            count += size_by_order[current->order] - sizeof(MallocMetadata);
        }
        else{
            count+= current->size;
        }
        current = current->next;
    }
    return count;
}

// Returns the overall number of metadata bytes currently in use
size_t BuddyAllocator::_num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

// Returns the size of a single metadata structure
size_t BuddyAllocator::_size_meta_data() {
    return sizeof(MallocMetadata);
}


void BuddyAllocator::insert_to_allocated(MallocMetadata *to_add) {
    // If the list is empty
    if (!tail_of_allocated) {
        tail_of_allocated = to_add;
        tail_of_allocated->next = nullptr;
        tail_of_allocated->prev = nullptr;
    } else {
        to_add->next = tail_of_allocated;
        tail_of_allocated->prev = to_add;
        tail_of_allocated = to_add;
    }
}



void BuddyAllocator::delete_from_allocated(MallocMetadata *mallocMetadata)  {
    MallocMetadata *toDelete = tail_of_allocated;
    if(!toDelete || !mallocMetadata){
        return;
    }
    while (toDelete){
        if(toDelete == mallocMetadata){
            break;
        }
        else {
            toDelete = toDelete->next;
        }
    }
    if(!toDelete){
        return;
    }

    // Find head
    MallocMetadata* tail = tail_of_allocated; // tail 1->2->3 head
    MallocMetadata* head = tail;
    while (head->next){
        head = head->next;
    }

    if(!toDelete->next && !toDelete->prev){ // If the size is with only one node
        tail_of_allocated = nullptr;
    }
    else if(toDelete == tail) // If it's the first node
    {
        tail->next->prev = nullptr;
        tail_of_allocated = tail->next;
    }
    else if(toDelete == head){ // If it's the last node
        head->prev->next = nullptr;
    }
    else { // If it's in the middle of the list
        toDelete->prev->next = toDelete->next;
        toDelete->next->prev = toDelete->prev;
    }
}

void BuddyAllocator::init_blocks() {

    // align to 32 blocks
    void* prog_brk = sbrk(0);
    if (prog_brk == (void*)(-1)){
        return;
    }
    if((unsigned long)prog_brk % (MAX_SIZE*32) !=0) {
        size_t offset = (MAX_SIZE * 32) - (unsigned long) prog_brk % (MAX_SIZE * 32);
        void *offsetAlloc = sbrk(offset);
        if (offsetAlloc == (void *) (-1)) {
            return;
        }
    }
    // create 32 blocks
    for (int i = 0; i < BLOCKS_NUM; ++i) {
        void* result = sbrk(MAX_SIZE);
        if (result == (void*)(-1)) {
            return;
        }
        MallocMetadata* new_metadata = (MallocMetadata*)result;
        new_metadata->size = MAX_SIZE;
        new_metadata->is_free = true;
        new_metadata->next = nullptr;
        new_metadata->prev = nullptr;
        new_metadata->block_index = i;
        new_metadata->order = MAX_ORDER;
        new_metadata->org_scalloc = false;
        appendToFreeData(new_metadata, MAX_ORDER);
    }
}

MallocMetadata *BuddyAllocator::mergeBlocksSrealloc(MallocMetadata *block, int order, size_t wanted_size) {
    //size_t block_size = pow(2, order);
    size_t block_size = size_by_order[order];
    // Calculate address of the buddy block
    void* buddyAddress = (void*)((unsigned long)block ^ block_size);
    MallocMetadata* buddy = (MallocMetadata*)(buddyAddress);
    if (buddy->is_free && buddy->order == block->order && buddy->block_index == block->block_index
        && block_size != MAX_SIZE && 2* block_size - sizeof(MallocMetadata)< wanted_size) {
        removeFromFreeData(block, block->order);
        removeFromFreeData(buddy, buddy->order);
        // Update size and order of merged block
        if(buddy < block){
            block = buddy;
        }
        block->order += 1;
        // Recursively merge if possible
        appendToFreeData(block, block->order);
        return mergeBlocksSrealloc(block, block->order, wanted_size);
    }
    if (buddy->is_free && buddy->order == block->order && buddy->block_index == block->block_index
        && block_size != MAX_SIZE && 2* block_size - sizeof(MallocMetadata)>=wanted_size) {
        removeFromFreeData(block, block->order);
        removeFromFreeData(buddy, buddy->order);
        // Update size and order of merged block
        if(buddy < block){
            block = buddy;
        }
        block->order += 1;
        // Recursively merge if possible
        appendToFreeData(block, block->order);
        return block;
    }
    return block;
}
size_t BuddyAllocator::find_huge_page_size(size_t size) {
      if(size % MAX_SCALLOC == 0){
        return size;
      }
      return ((size/MAX_SCALLOC) +1)* MAX_SCALLOC;
  }

BuddyAllocator bd = BuddyAllocator();

void* smalloc(size_t size){
    return bd.smalloc(size, false, size);
}

void sfree(void* p){
    return bd.sfree(p);
}
void* srealloc(void* oldp, size_t size){
    return bd.srealloc(oldp, size);
}

void* scalloc(size_t num, size_t size){
    return bd.scalloc(num, size);
}
size_t _num_free_blocks(){
    return bd._num_free_blocks();
}

size_t _num_free_bytes(){
    return bd._num_free_bytes();
}
size_t _num_allocated_blocks(){
    return bd._num_allocated_blocks();
}
size_t _num_allocated_bytes(){
    return bd._num_allocated_bytes();
}
size_t _num_meta_data_bytes(){
    return bd._num_meta_data_bytes();
}
size_t _size_meta_data(){
    return bd._size_meta_data();
}