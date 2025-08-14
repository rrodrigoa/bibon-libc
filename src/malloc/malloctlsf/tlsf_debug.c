#include "tlsf/platform_utils.h"

void debug_log_tlsf(const char* func, int line, int indent, const char* fmt, ...) {
    char msg[1024]; 
    int offset = 0;

    for (int i = 0; i < indent && offset < (int)sizeof(msg)-1; i++){
        msg[offset++] = ' ';
    }

    offset += snprintf(msg + offset, sizeof(msg) - offset, "[%s:%d] ", func, line);

    va_list args;
    va_start(args, fmt);
    offset += vsnprintf(msg + offset, sizeof(msg) - offset, fmt, args);
    va_end(args);

    if(offset < (int)sizeof(msg) -1){
        msg[offset++] = '\n';
    }

    write(STDERR_FILENO, msg, offset);
}