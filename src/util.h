#ifndef NHM_UTIL_H
#define NHM_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include <NickelHook.h>

// strtrim trims ASCII whitespace in-place (i.e. don't give it a string literal)
// from the left/right of the string.
__attribute__((unused)) static inline char *strtrim(char *s) {
    if (!s) return NULL;
    char *a = s, *b = s + strlen(s);
    for (; a < b && isspace((unsigned char)(*a)); a++);
    for (; b > a && isspace((unsigned char)(*(b-1))); b--);
    *b = '\0';
    return a;
}

// NHM_LOG writes a log message.
#define NHM_LOG(fmt, ...) nh_log(fmt " (%s:%d)", ##__VA_ARGS__, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif
#endif
