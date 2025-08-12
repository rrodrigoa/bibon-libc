#ifndef TLSF_HEADER
#define TLSF_HEADER

#include "platform_utils.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define TLSF_BLOCK_SIZE 16

#define IS_FREE_BITMASK_BLOCK 0x1
#define IS_LAST_PHYSICAL_BLOCK 0x2
#define IS_ALIGNED_MEMORY_BLOCK 0x4

#define TLSF_J 4
#define TLSF_2_POWER_J (1 << TLSF_J)

#define FL_BITMAP_SIZE sizeof(uint32_t) * 8
#define SL_BITMAP_SIZE (1 << TLSF_J)

#define TLSF_SPLIT_THRESHOLD (1024*10)

static bool TLSF_Initialized = false;

struct BlockHeader {
    struct BlockHeader* previousPhysicalBlock;
    uint32_t size;
    uint32_t bitMask;
    struct BlockHeader* nextFreeBlock;
    struct BlockHeader* previousFreeBlock;
};

struct ControlBlock {
    struct BlockHeader* block_null;
    uint32_t fl_bitmap;
    uint32_t sl_bitmap[FL_BITMAP_SIZE];

    // The blocks of memory allocated for the pool
    struct BlockHeader* blocks[FL_BITMAP_SIZE][SL_BITMAP_SIZE];
};

struct ControlBlock* TLSF_InitializeControlBlock(uint32_t size);
void TLSF_DestroyControlBlock(struct ControlBlock* control);

void* TLSF_malloc(struct ControlBlock* control, size_t size);
void* TLSF_realloc(struct ControlBlock* control, void* address, size_t new_size);
void* TLSF_memalign(struct ControlBlock* control, size_t alignment, size_t size);
void TLSF_free(struct ControlBlock* control, void* address);

// Insert support
void _mapping_insert(struct BlockHeader* block, uint32_t *fl, uint32_t *sl);
void _insert_block(struct ControlBlock* control, struct BlockHeader* block, const uint32_t *fl, const uint32_t *sl);

// Mapping and FL SL search
void _mapping_search(uint32_t *bytes, uint32_t *fl, uint32_t *sl);
struct BlockHeader* _find_suitable_block(struct ControlBlock *control, uint32_t *fl, uint32_t *sl);

// Remove block support
void _remove_head(struct ControlBlock* control, const uint32_t* fl, const uint32_t* sl);
void _remove_block(struct ControlBlock* control, struct BlockHeader* block, const uint32_t* fl, const uint32_t* sl);

// Split block
struct BlockHeader* _split(struct BlockHeader* block, const uint32_t* bytes);

// Merge support
struct BlockHeader* _merge_prev(struct ControlBlock* control, struct BlockHeader* block);
struct BlockHeader* _merge_next(struct ControlBlock* control, struct BlockHeader* block);
void _merge(struct BlockHeader* prevBlock, struct  BlockHeader* block);

// Physical block
struct BlockHeader* _nextPhysicalBlockAddress(struct BlockHeader* block);
bool _is_last_physical_block(struct BlockHeader* block);
void _set_last_physical_block(struct BlockHeader* block, bool value);

// Free block
void _set_free_block(struct BlockHeader* block, bool value);
bool _is_free_block(struct BlockHeader* block);

// Block size and pointers
uint32_t _block_size(struct BlockHeader* block);
struct BlockHeader* _get_block_from_pointer(void* address);

// Alignment support
void _set_aligned_block(struct BlockHeader* block, bool value);
bool _is_aligned_block(struct BlockHeader* block);
uint8_t* _align_up(uint8_t* address, size_t alignment);

void* __libc_malloc(size_t size);
void* __libc_calloc(size_t nmeb, size_t size);
void* __libc_realloc(void* ptr, size_t new_size);
void* __libc_memalign(size_t size, size_t align);
void __libc_free(void* ptr);
void __malloc_donate(void* ptr);

#endif
