#include "platform_utils.h"

// Bit Scan Reverse for 32 bits data for WIN and Linux
bool _bit_scan_reverse_32(uint32_t value, unsigned long* index) {
#if defined(_WIN32)
    return _BitScanReverse(index, value);
#else
    if (value == 0) return false;
    *index = 31 - __builtin_clz(value);  // count leading zeros
    return true;
#endif
}

// Bit Scan forward for 32 bits data for WIN and Linux
bool _bit_scan_forward_32(uint32_t value, unsigned long* index) {
#if defined(_WIN32)
    return _BitScanForward(index, value);
#else
    if (value == 0) return false;
    *index = __builtin_ctz(value);  // count trailing zeros
    return true;
#endif
}

// Bit scan reverse for 64 bits data for WIN and Linux
bool _bit_scan_reverse_64(uint64_t value, unsigned long* index) {
#if defined(_WIN32)
    return _BitScanReverse64(index, value);
#else
    if (value == 0) return false;
    *index = 63 - __builtin_clzll(value);
    return true;
#endif
}

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>

    void* tlsf_mmap(size_t size) {
        HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, NULL);
        if (!hMap) return NULL;
        void* ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
        CloseHandle(hMap); // Optional: close handle, since view remains valid
        return ptr;
    }

    void tlsf_munmap(void* ptr, size_t size) {
        UnmapViewOfFile(ptr);
    }
#else
    #include <sys/mman.h>
    #include <unistd.h>

    void* tlsf_mmap(size_t size) {
        return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    void tlsf_munmap(void* ptr, size_t size) {
        munmap(ptr, size);
    }
#endif