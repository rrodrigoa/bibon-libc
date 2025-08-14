#include "tlsf/tlsf.h"
#include "tlsf/platform_utils.h"
#include "tlsf/tlsf_debug.h"
#include <string.h>

TLSF_DEBUG_INDENT(int indent_tlsf = 0);

struct ControlBlock* TLSF_InitializeControlBlock(uint32_t size) {
    uint32_t sizeInBytes = (uint32_t)(1 << size);

    if (sizeInBytes <= 0) {
        TLSF_DEBUG_LOG(indent_tlsf, "InitializeControlBlock sizeInBytes <= 0");
        return NULL;
    }

    void* firstBlockMemory = tlsf_mmap(sizeInBytes + sizeof(struct BlockHeader));
    if (firstBlockMemory == NULL) {
        TLSF_DEBUG_LOG(indent_tlsf, "First block memory is NULL");
        return NULL;
    }

    TLSF_DEBUG_LOG(indent_tlsf, "Allocated block of size %d on address %p", size, firstBlockMemory);

    struct BlockHeader* firstBlock = (struct BlockHeader*)firstBlockMemory;
    memset(firstBlock, 0, sizeof(struct BlockHeader));
    firstBlock->size = sizeInBytes - sizeof(struct BlockHeader);
    _set_last_physical_block(firstBlock, true);

    TLSF_DEBUG_LOG(indent_tlsf, "Set memoty block to zero for size %u on address %p", firstBlock->size, firstBlockMemory);

    void* controlBlockPointer = tlsf_mmap(sizeof(struct ControlBlock));
    if (controlBlockPointer == NULL) {
        TLSF_DEBUG_LOG(indent_tlsf, "Malloc returned NULL for controll block");
        return NULL;
    }
    memset(controlBlockPointer, 0, sizeof(struct ControlBlock));
    TLSF_DEBUG_LOG(indent_tlsf, "Set control block to zero for size %u", sizeof(struct ControlBlock));
    struct ControlBlock* controlBlock = (struct ControlBlock*)controlBlockPointer;

    uint32_t fl, sl;
    _mapping_insert(firstBlock, &fl, &sl);
    _insert_block(controlBlock, firstBlock, &fl, &sl);

    TLSF_DEBUG_LOG(indent_tlsf, "Control created, returning it");
    return controlBlock;
}

void TLSF_DestroyControlBlock(struct ControlBlock* control) {
    // TODO: destroy every struct BlockHeader allocated
    tlsf_munmap(control, sizeof(struct ControlBlock));
}

void* TLSF_malloc(struct ControlBlock* control, size_t size) {
    uint32_t fl, sl;
    uint32_t bytes = size;

    if (size == 0) {
        TLSF_DEBUG_LOG(indent_tlsf, "Returning NULL");
        return NULL;
    }

    _mapping_search(&bytes, &fl, &sl);

    struct BlockHeader* free_block = _find_suitable_block(control, &fl, &sl);
    
    if (free_block == NULL || free_block->size - sizeof(struct BlockHeader) < bytes) {
        TLSF_DEBUG_LOG(indent_tlsf, "Cannot return a valid block, returning NULL");
        return NULL;
    }

    _remove_head(control, &fl, &sl);
    if (_block_size(free_block) - bytes > (uint32_t)TLSF_SPLIT_THRESHOLD) {
        struct BlockHeader* remaining_block = (struct BlockHeader*)_split(free_block, &bytes);
        _mapping_insert(remaining_block, &fl, &sl);
        _insert_block(control, remaining_block, &fl, &sl);
    }

    _set_free_block(free_block, false);
    uint8_t* pointer = (uint8_t*)free_block;
    pointer += sizeof(struct BlockHeader);

    TLSF_DEBUG_LOG(indent_tlsf, "Return a valid memory block on address %p", pointer);
    return (void*)pointer;
}

void* TLSF_realloc(struct ControlBlock* control, void* address, size_t new_size) {
    if (new_size == 0) {
        TLSF_DEBUG_LOG(indent_tlsf, "Returning null");
        return NULL;
    }

    void* new_mem = TLSF_malloc(control, new_size);
    struct BlockHeader* old_block = _get_block_from_pointer(address);
    size_t old_block_size = _block_size(old_block);

    memcpy(new_mem, address, MIN(new_size, old_block_size));
    TLSF_free(control, address);

    TLSF_DEBUG_LOG(indent_tlsf, "Returning memory on block %p", new_mem);
    return new_mem;
}

void* TLSF_memalign(struct ControlBlock* control, size_t size, size_t alignment) {
    if (size == 0)
        return NULL;

    uint32_t bytes = size + alignment - 1 + sizeof(void*);

    void* free_memory = TLSF_malloc(control, bytes);
    struct BlockHeader* block = _get_block_from_pointer(free_memory);

    uint8_t* pointer_first_byte = (uint8_t*)block;
    uint8_t* pointer_first_space = pointer_first_byte + sizeof(struct BlockHeader) + sizeof(void*);
    uint8_t* pointer_first_aligned_space = _align_up(pointer_first_space, alignment);

    struct BlockHeader copy_header;
    memcpy(&copy_header, (void*)block, sizeof(struct BlockHeader));
    uint8_t* point_to_new_block = pointer_first_aligned_space - sizeof(struct BlockHeader);
    memcpy((void*)point_to_new_block, (void*)&copy_header, sizeof(struct BlockHeader));
    
    void** point_to_back_pointer = (void**)(point_to_new_block - sizeof(void*));
    *point_to_back_pointer = (void*)pointer_first_byte;

    struct BlockHeader* new_block = (struct BlockHeader*)point_to_new_block;
    _set_aligned_block(new_block, true);

    return (void*)pointer_first_aligned_space;
}

void TLSF_free(struct ControlBlock* control, void* address)
{
    uint8_t* block_pointer = (uint8_t*)address;
    block_pointer -= sizeof(struct BlockHeader);

    struct BlockHeader* block = (struct BlockHeader*)block_pointer;

    if (_is_aligned_block(block)) {
        void** back_pointer = (void**)(block_pointer - sizeof(void*));
        void* all_start = *back_pointer;

        struct BlockHeader copy_header;
        memcpy(&copy_header, block, sizeof(struct BlockHeader));
        memcpy(all_start, &copy_header, sizeof(struct BlockHeader));
        block = (struct BlockHeader*)all_start;
        _set_aligned_block(block, false);
    }

    struct BlockHeader* merged_block = _merge_prev(control, block);
    merged_block = _merge_next(control, merged_block);

    uint32_t fl, sl;
    _mapping_insert(merged_block, &fl, &sl);
    _insert_block(control, merged_block, &fl, &sl);
    address = NULL;
}

void _mapping_insert(struct BlockHeader* block, uint32_t* fl, uint32_t* sl) {
    unsigned long longNumber;
    unsigned long bytes = (unsigned long)block->size;
    _bit_scan_reverse_64(bytes, &longNumber);
    *fl = (uint32_t)longNumber;
    *sl = (block->size >> (*fl - TLSF_J)) - TLSF_2_POWER_J;
}

inline void _set_free_block(struct BlockHeader* block, bool value) {
    if (value) {
        block->bitMask |= IS_FREE_BITMASK_BLOCK;
    } else {
        block->bitMask &= ~IS_FREE_BITMASK_BLOCK;
    }
}

inline bool _is_free_block(struct BlockHeader* block) {
    return block->bitMask && IS_FREE_BITMASK_BLOCK;
}

inline void _set_last_physical_block(struct BlockHeader* block, bool value) {
    if (value) {
        block->bitMask |= IS_LAST_PHYSICAL_BLOCK;
    }
    else {
        block->bitMask &= ~IS_LAST_PHYSICAL_BLOCK;
    }
}
inline bool _is_last_physical_block(struct BlockHeader* block) {
    return block->bitMask & IS_LAST_PHYSICAL_BLOCK;
}

inline void _set_aligned_block(struct BlockHeader* block, bool value) {
    if (value) {
        block->bitMask |= IS_ALIGNED_MEMORY_BLOCK;
    }
    else {
        block->bitMask &= ~IS_ALIGNED_MEMORY_BLOCK;
    }
}

inline bool _is_aligned_block(struct BlockHeader* block) {
    return block->bitMask & IS_ALIGNED_MEMORY_BLOCK;
}

inline uint32_t _block_size(struct BlockHeader* block) {
    return block->size;
}

inline struct BlockHeader* _get_block_from_pointer(void* address) {
    uint8_t* block_address = (uint8_t*)address - sizeof(struct BlockHeader);
    struct BlockHeader* block = (struct BlockHeader*)block_address;

    return block;
}

void _insert_block(struct ControlBlock* control, struct BlockHeader* block, const uint32_t* fl, const uint32_t* sl) {
    block->nextFreeBlock = control->blocks[*fl][*sl];
    control->blocks[*fl][*sl] = block;

    if (block->nextFreeBlock != NULL) {
        block->nextFreeBlock->previousFreeBlock = block;
    }

    _set_free_block(block, true);

    control->fl_bitmap |= 1 << *fl;
    control->sl_bitmap[*fl] |= 1 << *sl;

    struct BlockHeader* next_phys = (struct BlockHeader*)(block + _block_size(block));
    if (_is_last_physical_block(block) == false && next_phys != NULL) {
        next_phys->previousPhysicalBlock = block;
    }
}

void _mapping_search(uint32_t* bytes, uint32_t* fl, uint32_t* sl) {
    if (*bytes < (1u << TLSF_J)) {
        *fl = 0;
        *sl = MIN((uint32_t)*bytes, (uint32_t)(TLSF_2_POWER_J - 1));
        *bytes = 1u << TLSF_J;
        return;
    }

    unsigned long indexLeft;
    _bit_scan_reverse_32((unsigned long)*bytes, &indexLeft);

    *bytes += (1 << (indexLeft - TLSF_J)) - 1;
    _bit_scan_reverse_32((unsigned long)*bytes, &indexLeft);
    *fl = (uint32_t)indexLeft;
    *sl = (*bytes >> (*fl - TLSF_J)) - TLSF_2_POWER_J;
}

struct BlockHeader* _find_suitable_block(struct ControlBlock* control, uint32_t* fl, uint32_t* sl) {
    uint32_t bitmap_temp = control->sl_bitmap[*fl] & ((uint32_t)(~0) << *sl);
    uint32_t non_empty_sl, non_empty_fl;
    unsigned long longNumber;

    if (bitmap_temp != 0) {
        _bit_scan_forward_32((unsigned long)bitmap_temp, &longNumber);
        non_empty_sl = (unsigned int)longNumber;
        non_empty_fl = *fl;
    }
    else {
        bitmap_temp = control->fl_bitmap & ((uint32_t)(~0) << (*fl + 1));
        _bit_scan_forward_32((unsigned long)bitmap_temp, &longNumber);
        non_empty_fl = (uint32_t)longNumber;
        _bit_scan_forward_32((unsigned long)control->sl_bitmap[non_empty_fl], &longNumber);
        non_empty_sl = (uint32_t)longNumber;
    }

    struct BlockHeader* block = control->blocks[non_empty_fl][non_empty_sl];
    *fl = non_empty_fl;
    *sl = non_empty_sl;

    return block;
}

void _remove_head(struct ControlBlock* control, const uint32_t* fl, const uint32_t* sl) {
    struct BlockHeader* head = control->blocks[*fl][*sl];
    struct BlockHeader* headNext = head->nextFreeBlock;

    if (head->nextFreeBlock != NULL) {
        head->nextFreeBlock->previousFreeBlock = head->previousFreeBlock;
    }

    if (head->previousFreeBlock != NULL) {
        head->previousFreeBlock->nextFreeBlock = head->nextFreeBlock;
    }

    head->nextFreeBlock = NULL;
    head->previousFreeBlock = NULL;
    _set_free_block(head, false);
    control->blocks[*fl][*sl] = headNext;

    if (headNext == NULL) {
        bool isLast = true;
        control->sl_bitmap[*fl] &= ~(1 << *sl);
        for (int indexSL = 0; indexSL < TLSF_2_POWER_J; indexSL++) {
            if (control->blocks[*fl][indexSL] != NULL) {
                isLast = false;
                break;
            }
        }

        if (isLast) {
            control->fl_bitmap &= ~(1 << *fl);
        }
    }
}

struct BlockHeader* _split(struct BlockHeader* block, const uint32_t* bytes) {
    uint8_t* pointerToNewAddress = (uint8_t*)block;
    pointerToNewAddress += sizeof(struct BlockHeader) + *bytes;

    struct BlockHeader* remaining_block = (struct BlockHeader*)pointerToNewAddress;
    memset(remaining_block, 0, sizeof(struct BlockHeader));

    bool T = _is_last_physical_block(block);
    remaining_block->size = _block_size(block) - sizeof(struct BlockHeader) - *bytes;
    block->size = *bytes;

    if (T) {
        _set_last_physical_block(remaining_block, true);
        _set_last_physical_block(block, false);
    }
    _set_free_block(remaining_block, true);
    _set_free_block(block, true);

    remaining_block->previousPhysicalBlock = block;

    return remaining_block;
}

void _remove_block(struct ControlBlock* control, struct BlockHeader* block, const uint32_t* fl, const uint32_t* sl) {
    struct BlockHeader* blockNext = block->nextFreeBlock;
    struct BlockHeader* blockPrev = block->previousFreeBlock;

    if (control->blocks[*fl][*sl] == block) {
        control->blocks[*fl][*sl] = blockNext;
    }

    if (blockNext != NULL) {
        blockNext->previousFreeBlock = blockPrev;
    }
    if (blockPrev != NULL) {
        blockPrev->nextFreeBlock = blockNext;
    }
}

struct BlockHeader* _merge_prev(struct ControlBlock* control, struct BlockHeader* block) {
    struct BlockHeader* prevBlock = block->previousPhysicalBlock;

    if (prevBlock != NULL && _is_free_block(prevBlock)) {
        uint32_t fl, sl;
        _mapping_insert(prevBlock, &fl, &sl);
        _remove_block(control, prevBlock, &fl, &sl);
        _merge(prevBlock, block);
    }
    else {
        prevBlock = block;
    }

    return prevBlock;
}

struct BlockHeader* _merge_next(struct ControlBlock* control, struct BlockHeader* block) {
    uint8_t* pointer = ((uint8_t*)block) + sizeof(struct BlockHeader) + block->size;
    struct BlockHeader* nextBlock = (struct BlockHeader*)pointer;

    if (_is_last_physical_block(block) == false && _is_free_block(nextBlock)) {
        uint32_t fl, sl;
        _mapping_insert(nextBlock, &fl, &sl);
        _remove_block(control, nextBlock, &fl, &sl);
        _merge(block, nextBlock);
    }

    return block;
}

void _merge(struct BlockHeader* prevBlock, struct BlockHeader* block) {
    prevBlock->size = prevBlock->size + sizeof(struct BlockHeader) + block->size;

    if (_is_last_physical_block(block) == false) {
        struct BlockHeader* nextPhysicalBlock = _nextPhysicalBlockAddress(block);
        nextPhysicalBlock->previousPhysicalBlock = prevBlock;
    }
    else {
        _set_last_physical_block(prevBlock, true);
    }
}

struct BlockHeader* _nextPhysicalBlockAddress(struct BlockHeader* block) {
    return (struct BlockHeader*)((uint8_t*)block + sizeof(struct BlockHeader) + block->size);
}

uint8_t* _align_up(uint8_t* address, size_t alignment) {
    uint8_t* starting_address = (uint8_t*)address;

    uintptr_t aligned_address = (uintptr_t)(starting_address + (alignment - 1));
    aligned_address &= ~(alignment - 1);

    return (uint8_t*)aligned_address;
}


struct ControlBlock* processSharedControlBlock = NULL;

void* __libc_malloc(size_t size){
    if (!TLSF_Initialized) {
        processSharedControlBlock = TLSF_InitializeControlBlock(TLSF_BLOCK_SIZE);
    }

    void* mem = TLSF_malloc(processSharedControlBlock, size);
    return mem;
}

void* __libc_calloc(size_t nmeb, size_t size) {
    void* mem = __libc_malloc(size * nmeb);
    memset(mem, 0, size * nmeb);
    return mem;
}

void* __libc_realloc(void* ptr, size_t new_size) {
    return TLSF_realloc(processSharedControlBlock, ptr, new_size);
}

void* __libc_memalign(size_t size, size_t align) {
    return TLSF_memalign(processSharedControlBlock, size, align);
}

void __libc_free(void* ptr){
    TLSF_free(processSharedControlBlock, ptr);
}

void __malloc_donate(void* ptr){
    TLSF_free(processSharedControlBlock, ptr);
}

int __malloc_replaced = 1;
int __aligned_alloc_replaced = 1;

void* malloc(size_t size){
    return __libc_malloc(size);
}
void* realloc(void* ptr, size_t new_size){
    return __libc_realloc(ptr, new_size);
}
void* calloc(size_t nmeb, size_t size){
    return __libc_calloc(nmeb, size);
}
void* memalign(size_t size, size_t align){
    return __libc_memalign(size, align);
}
void free(void* ptr){
    __libc_free(ptr);
}