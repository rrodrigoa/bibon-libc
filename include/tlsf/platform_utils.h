#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H
    #include <stdint.h>
    #include <assert.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <stddef.h>
    
    // Define true and false boolean
    #include <stdbool.h>
    #define bool _Bool
    #define true 1
    #define false 0

    // Define own min(a, b) function
    #ifndef MIN
        #define MIN(a, b) ((a) < (b) ? (a) : (b))
    #endif

    // Alignment of code defined stack data
    #if defined(_MSC_VER)
    #define ALIGNAS(x) __declspec(align(x))
    #else
    #define ALIGNAS(x) __attribute__((aligned(x)))
    #endif

    // Adding MAP method properties
    #if defined(_WIN32)
        #include <intrin.h>
    #else
        #include <sys/mman.h>
        #ifndef MAP_ANONYMOUS
            #define MAP_ANONYMOUS 0x20
        #endif
    #endif

    // Bit Scan Reverse for 32 bits data for WIN and Linux
    bool _bit_scan_reverse_32(uint32_t value, unsigned long* index);

    // Bit Scan forward for 32 bits data for WIN and Linux
    bool _bit_scan_forward_32(uint32_t value, unsigned long* index);

    // Bit scan reverse for 64 bits data for WIN and Linux
    bool _bit_scan_reverse_64(uint64_t value, unsigned long* index);

    // Define FILE 2 for debugg and err message
    #if defined(_WIN32)
        #include <io.h>
        #define write _write
        #define STDERR_FILENO 2
    #else
        #include <unistd.h>
    #endif

    void* tlsf_mmap(size_t size);
    void tlsf_munmap(void* ptr, size_t size);

#endif // PLATFORM_UTILS_H

