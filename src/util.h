#ifndef NHM_UTIL_H
#define NHM_UTIL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <NickelHook.h>

// The mod version, baked in by NickelHook.mk (git describe). Logged on every line and in the
// startup block, so a user-attached log always says exactly which build produced it.
#ifndef NH_VERSION
#define NH_VERSION "dev"
#endif

// Cap the on-device log so it can't grow without bound across many boots. On the first write of
// a boot, if the log is larger than this it's rotated to a single ".old" generation. A healthy
// boot writes only the startup block, so this is reached only by a long-lived or verbose device.
#ifndef NHM_LOG_MAX_BYTES
#define NHM_LOG_MAX_BYTES (256 * 1024)
#endif

// Verbose logging for the boot: nhm_log:1 in the config, or a config problem was detected.
// Published by config.c at the end of the parse (see config.c / config.h).
extern bool nhm_log_verbose;
bool nhm_config_problem_seen(void);

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

// nhm_log_file_line writes a message to both syslog (nh_log, via `logread`) and a persistent
// file at NHM_CONFIG_DIR "/nickel-home.log" on the user partition, so users can attach a log
// over USB without shell access.
__attribute__((unused)) static inline void nhm_log_file_line(const char *file, int line, const char *fmt, ...) {
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    msg[sizeof(msg) - 1] = '\0';

    nh_log("%s (%s:%d)", msg, file, line);

    mkdir(NHM_CONFIG_DIR, 0755);

    // Rotate once per process, on the first file write of the boot. A benign race if two threads
    // hit this first (at most a redundant rename); the flag keeps it to one check per process.
    static bool nhm_log_rotate_checked = false;
    if (!nhm_log_rotate_checked) {
        nhm_log_rotate_checked = true;
        struct stat st;
        if (stat(NHM_CONFIG_DIR "/nickel-home.log", &st) == 0 && st.st_size > NHM_LOG_MAX_BYTES)
            rename(NHM_CONFIG_DIR "/nickel-home.log", NHM_CONFIG_DIR "/nickel-home.log.old");
    }

    FILE *f = fopen(NHM_CONFIG_DIR "/nickel-home.log", "a");
    if (!f)
        return;

    // localtime_r, not localtime: the shared static buffer is a data race between threads.
    time_t now = time(NULL);
    struct tm tmbuf;
    struct tm *tm = localtime_r(&now, &tmbuf);
    if (tm) {
        fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d ",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    }

    fprintf(f, "NickelHome " NH_VERSION ": %s (%s:%d)\n", msg, file, line);
    fclose(f);
}

// NHM_LOG: always written (problems, state changes, startup). A healthy boot writes only the
// startup block at this level.
#define NHM_LOG(fmt, ...) nhm_log_file_line(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

// NHM_DBG: verbose tracing, written only with nhm_log:1 or after a config problem was detected.
#define NHM_DBG(fmt, ...) do { \
        if (nhm_log_verbose || nhm_config_problem_seen()) \
            NHM_LOG(fmt, ##__VA_ARGS__); \
    } while (0)

// nhm_log_firmware logs the running firmware version once at startup, next to the mod version
// and resolved hook, so a future-firmware breakage report shows which firmware ran. The serial
// number (field 0 of /mnt/onboard/.kobo/version) is deliberately dropped. Failures are silent.
__attribute__((unused)) static inline void nhm_log_firmware(void) {
    FILE *f = fopen("/mnt/onboard/.kobo/version", "r");
    if (!f) {
        NHM_LOG("startup: firmware version unavailable");
        return;
    }
    char vline[512];
    char *got = fgets(vline, sizeof(vline), f);
    fclose(f);
    if (!got)
        return;
    vline[strcspn(vline, "\r\n")] = '\0';
    const char *comma = strchr(vline, ',');   // <serial>,<...>,<firmware>,<model>,...
    NHM_LOG("startup: firmware %s", comma ? comma + 1 : vline);
}

#ifdef __cplusplus
}
#endif
#endif
