/*
 * bibon_malloc_free_tests.c
 *
 * Ten targeted tests for malloc/free behavior in bibon-libc.
 * - Each test explains the expectation in comments.
 * - Uses asserts for hard pass/fail.
 * - Includes optional address-reuse checks (off by default) because not every
 * allocator guarantees reuse order.
 *
 * Build:
 *   cc -O0 -g -Wall -Wextra -std=c11 bibon_malloc_free_tests.c -o bibon_tests
 * Run:
 *   ./bibon_tests
 *
 * Optional knobs (define at compile time, e.g. -DBIBON_STRICT_REUSE=1):
 *   BIBON_STRICT_REUSE     -> if 1, fail when a same-size allocation after free
 * does NOT return the same address. BIBON_VERBOSE          -> if 1, print extra
 * progress info.
 */

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Config knobs ------------------------------------------------------- */
#ifndef BIBON_STRICT_REUSE
#define BIBON_STRICT_REUSE \
    0 /* set to 1 to *require* address reuse in tests that check it */
#endif

#ifndef BIBON_VERBOSE
#define BIBON_VERBOSE 1
#endif

#define LOG(...)                 \
    do {                         \
        if (BIBON_VERBOSE) {     \
            printf(__VA_ARGS__); \
        }                        \
    } while (0)

/* ---- Small helpers ------------------------------------------------------ */

/* Simple xorshift32 for deterministic pseudo-random data (no libc rand()) */
static uint32_t prng_state = 0xC0FFEEu;
static inline uint32_t xorshift32(void) {
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = x;
    return x;
}

/* Fill a buffer with a deterministic pattern and verify it later */
static void fill_pattern(uint8_t *p, size_t n, uint32_t seed) {
    uint32_t s = seed;
    for (size_t i = 0; i < n; ++i) {
        /* cheap, stable pattern */
        s = s * 1664525u + 1013904223u;
        p[i] = (uint8_t)(s >> 24);
    }
}

static void check_pattern(const uint8_t *p, size_t n, uint32_t seed) {
    uint32_t s = seed;
    for (size_t i = 0; i < n; ++i) {
        s = s * 1664525u + 1013904223u;
        uint8_t expected = (uint8_t)(s >> 24);
        if (p[i] != expected) {
            fprintf(stderr,
                    "Pattern mismatch at idx=%zu: got=0x%02X expected=0x%02X\n",
                    i, p[i], expected);
            assert(0 && "memory pattern corrupted");
        }
    }
}

/* Alignment check: C says malloc returns a pointer suitably aligned for any
   object type. Here we assert alignment >= alignof(max_align_t). */
static void assert_max_alignment(void *ptr) {
    size_t align = _Alignof(max_align_t);
    assert(((uintptr_t)ptr % align) == 0 &&
           "malloc result not max_align_t-aligned");
}

/* Soft check for address reuse (by size). Some allocators DO reuse, some MAY
   NOT. If BIBON_STRICT_REUSE=1, we assert equality; otherwise we log a warning.
 */
static void check_address_reuse(void *oldp, void *newp, size_t sz) {
    if (oldp == newp) {
        LOG("  Address reuse OK for %zu bytes (ptr=%p)\n", sz, newp);
        return;
    }
#if BIBON_STRICT_REUSE
    fprintf(stderr, "Expected address reuse for size %zu: old=%p new=%p\n", sz,
            oldp, newp);
    assert(0 && "allocator did not reuse freed block as expected");
#else
    LOG("  Address reuse not observed for %zu bytes (old=%p new=%p) — allowed "
        "(STRICT_REUSE=0)\n",
        sz, oldp, newp);
#endif
}

/* ---- Tests -------------------------------------------------------------- */

/* 1) Basic non-NULL allocation/free and alignment */
static void test_basic_alloc_free(void) {
    LOG("[1] Basic alloc/free & alignment\n");
    size_t n = 256;
    void *p = malloc(n);
    assert(p != NULL && "malloc(256) returned NULL");
    assert_max_alignment(p);
    memset(p, 0xAA, n);
    free(p);
}

/* 2) Two allocations must return distinct, non-overlapping pointers
 * (simplified: just !=) */
static void test_two_allocs_distinct(void) {
    LOG("[2] Two distinct allocations\n");
    void *a = malloc(128);
    void *b = malloc(128);
    assert(a != NULL && b != NULL);
    assert(a != b &&
           "two mallocs of same size returned same pointer (overlap bug?)");
    free(a);
    free(b);
}

/* 3) Write-read integrity: fill pattern, verify, free */
static void test_write_read_pattern(void) {
    LOG("[3] Write/Read integrity\n");
    size_t n = 4096;
    uint8_t *p = (uint8_t *)malloc(n);
    assert(p != NULL);
    fill_pattern(p, n, 0x12345678u);
    check_pattern(p, n, 0x12345678u);
    free(p);
}

/* 4) Free -> same-size allocation reuses address (soft check; can enforce with
 * STRICT_REUSE) */
static void test_free_reuse_same_size(void) {
    LOG("[4] Free then same-size alloc (address reuse expected/preferred)\n");
    size_t n = 1024;
    void *a = malloc(n);
    assert(a != NULL);
    memset(a, 0x5A, n);
    free(a);
    void *b = malloc(n);
    assert(b != NULL);
    check_address_reuse(a, b, n);
    free(b);
}

/* 5) Middle free reuse: allocate A,B,C; free B; allocate size(B) and prefer
 * address==B */
static void test_middle_free_reuse(void) {
    LOG("[5] Reuse of a freed middle block\n");
    size_t sa = 512, sb = 1536, sc = 512;
    void *A = malloc(sa);
    void *B = malloc(sb);
    void *C = malloc(sc);
    assert(A && B && C);

    memset(A, 0x11, sa);
    memset(B, 0x22, sb);
    memset(C, 0x33, sc);

    free(B);

    void *B2 = malloc(sb);
    assert(B2 != NULL);
    check_address_reuse(B, B2, sb);

    free(A);
    free(C);
    free(B2);
}

/* 6) Alignment guarantee on various sizes */
static void test_alignment_various_sizes(void) {
    LOG("[6] Alignment across a set of sizes\n");
    size_t sizes[] = {1, 2, 3, 7, 8, 16, 24, 32, 48, 64, 96, 128, 256, 1024};
    void *ptrs[sizeof(sizes) / sizeof(sizes[0])] = {0};

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        ptrs[i] = malloc(sizes[i]);
        assert(ptrs[i] != NULL);
        assert_max_alignment(ptrs[i]);
    }
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        free(ptrs[i]);
    }
}

/* 7) free(NULL) must be a no-op (never crash) */
static void test_free_null_noop(void) {
    LOG("[7] free(NULL) is a no-op\n");
    void *p = NULL;
    free(p); /* should NOT crash */
}

/* 8) Zero-size malloc: behavior is implementation-defined.
      Expectation: either NULL or a unique pointer safe to pass to free(). We
   assert only "no crash". */
static void test_zero_size_malloc(void) {
    LOG("[8] Zero-size malloc behavior (no crash; pointer may be NULL or "
        "not)\n");
    void *p = malloc(0);
    /* Either is allowed. If non-NULL, it must be free-able. */
    if (p != NULL) {
        assert_max_alignment(p);
        free(p);
    }
}

/* 9) Many small allocs with random sizes, then free in randomized order while
 * checking data integrity */
static void test_many_small_random(void) {
    LOG("[9] Many small randomized allocations with integrity checks\n");
    enum { N = 4000 };
    uint8_t *blocks[N] = {0};
    uint16_t sizes[N];

    /* allocate with sizes in [1, 256] and fill patterns */
    for (int i = 0; i < N; ++i) {
        uint16_t sz = (uint16_t)((xorshift32() % 256u) + 1u);
        sizes[i] = sz;
        blocks[i] = (uint8_t *)malloc(sz);
        assert(blocks[i] != NULL);
        assert_max_alignment(blocks[i]);
        fill_pattern(blocks[i], sz, 0xA5A5A5A5u ^ (uint32_t)i);
    }

    /* verify before free */
    for (int i = 0; i < N; ++i) {
        check_pattern(blocks[i], sizes[i], 0xA5A5A5A5u ^ (uint32_t)i);
    }

    /* shuffle indices with Fisher-Yates using our PRNG */
    int idx[N];
    for (int i = 0; i < N; ++i)
        idx[i] = i;
    for (int i = N - 1; i > 0; --i) {
        int j = (int)(xorshift32() % (uint32_t)(i + 1));
        int tmp = idx[i];
        idx[i] = idx[j];
        idx[j] = tmp;
    }

    /* free in randomized order */
    for (int k = 0; k < N; ++k) {
        int i = idx[k];
        free(blocks[i]);
        blocks[i] = NULL;
    }
}

/* 10) Large allocation boundary touch: ensure the allocator can serve a big
 * chunk and we can touch its edges */
static void test_large_allocation(void) {
    LOG("[10] Large allocation + boundary writes\n");
    size_t n = 10 * 1024 * 1024; /* 10 MiB — adjust if your test box is tiny */
    uint8_t *p = (uint8_t *)malloc(n);
    assert(p != NULL && "large malloc failed");
    assert_max_alignment(p);

    /* Touch edges and a few interior spots to ensure pages are actually
     * mapped/writable */
    p[0] = 0xDE;
    p[n - 1] = 0xAD;
    p[n / 2] = 0xBE;
    p[n / 3] = 0xEF;

    assert(p[0] == 0xDE && p[n - 1] == 0xAD && p[n / 2] == 0xBE &&
           p[n / 3] == 0xEF);
    free(p);
}

/* ---- main --------------------------------------------------------------- */

int main(void) {
    LOG("=== bibon malloc/free test suite ===\n");

    test_basic_alloc_free();
    test_two_allocs_distinct();
    test_write_read_pattern();
    test_free_reuse_same_size();
    test_middle_free_reuse();
    test_alignment_various_sizes();
    test_free_null_noop();
    test_zero_size_malloc();
    test_many_small_random();
    test_large_allocation();

    LOG("All tests completed.\n");
    puts("OK");
    return 0;
}
