#ifndef NHM_CONFIG_H
#define NHM_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#if !(defined(NHM_CONFIG_DIR) && defined(NHM_CONFIG_DIR_DISP))
#error "NHM_CONFIG_DIR not set (it should be done by the Makefile)"
#endif

// Null-terminated list of the valid config keys, defined in nickelhome.cc next to the documented
// defaults so the two stay in sync. The parser warns about any key not in this list.
extern const char *const nhm_known_keys[];

// True once any config problem was seen (unknown key, malformed line, invalid value). A broken
// config forces verbose logging for the boot, so it diagnoses itself in the log (see util.h).
bool nhm_config_problem_seen(void);

typedef struct nhm_config_t nhm_config_t;

// nhm_config_parse reads and parses NHM_CONFIG_DIR "/config". Each non-empty,
// non-comment line is of the form `key:val`. If the config file does not exist,
// it is seeded (atomically) by copying the bundled template at NHM_CONFIG_DIR "/default" (the
// default "minimal" configuration) and then parsed. Malformed and unknown keys are logged.
// The returned config must be freed with nhm_config_free. Returns NULL only if the config
// struct itself could not be allocated; the getters below treat a NULL config as an empty one,
// so every key falls back to its default.
nhm_config_t *nhm_config_parse(void);

// nhm_config_get returns the value for the first declaration of key, or NULL if
// it is not set. The pointer is valid until nhm_config_free is called.
const char *nhm_config_get(nhm_config_t *cfg, const char *key);

// nhm_config_bool returns key as a boolean (1/0, true/false, yes/no, on/off), or default_value
// if unset. An invalid value logs a warning, flags a config problem, and returns default_value.
bool nhm_config_bool(nhm_config_t *cfg, const char *key, bool default_value);

// nhm_config_free frees a config returned by nhm_config_parse.
void nhm_config_free(nhm_config_t *cfg);

// Global accessors: parse the config on first use (caching it) and read a key. The returned
// pointer / value remains valid for the lifetime of the process. Prime from init before the
// hook can run.
const char *nhm_global_config_get(const char *key);
bool nhm_global_config_bool(const char *key, bool default_value);

#ifdef __cplusplus
}
#endif
#endif
