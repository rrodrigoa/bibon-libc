#ifndef TLSF_DEBUG_H
#define TLSF_DEBUG_H
    // Debug main method for TLSF
    void debug_log_tlsf(const char* func, int line, int indent, const char* fmt, ...);

    #ifdef TLSF_DEBUG
        #define TLSF_DEBUG_INDENT(x) x
        #define TLSF_DEBUG_INDENTADD(x) x++
        #define TLSF_DEBUG_INDENTSUB(x) x--
        #define TLSF_DEBUG_LOG(indent, ...) debug_log_tlsf(__func__, __LINE__, indent, __VA_ARGS__)
    #else
        #define TLSF_DEBUG_INDENT(x)
        #define TLSF_DEBUG_INDENTADD(x)
        #define TLSF_DEBUG_INDENTSUB(x)
        #define TLSF_DEBUG_LOG(...)
    #endif
#endif