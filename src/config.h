#ifndef NHM_CONFIG_H
#define NHM_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !(defined(NHM_CONFIG_DIR) && defined(NHM_CONFIG_DIR_DISP))
#error "NHM_CONFIG_DIR not set (it should be done by the Makefile)"
#endif

typedef struct nhm_config_t nhm_config_t;

// nhm_config_parse reads and parses NHM_CONFIG_DIR "/config". Each non-empty,
// non-comment line is of the form `key:val`. If the config file does not exist,
// it is seeded by copying the bundled template at NHM_CONFIG_DIR "/default" (the
// default "minimal" configuration) and then parsed. Malformed lines are logged
// and skipped.
// The returned config must be freed with nhm_config_free. This never returns NULL.
nhm_config_t *nhm_config_parse(void);

// nhm_config_get returns the value for the first declaration of key, or NULL if
// it is not set. The pointer is valid until nhm_config_free is called.
const char *nhm_config_get(nhm_config_t *cfg, const char *key);

// nhm_config_free frees a config returned by nhm_config_parse.
void nhm_config_free(nhm_config_t *cfg);

// nhm_global_config_get parses the config on first use (caching it) and returns
// the value for the first declaration of key, or NULL if it is not set. The
// pointer remains valid for the lifetime of the process.
const char *nhm_global_config_get(const char *key);

#ifdef __cplusplus
}
#endif
#endif
