#ifndef HM_CONFIG_H
#define HM_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !(defined(HM_CONFIG_DIR) && defined(HM_CONFIG_DIR_DISP))
#error "HM_CONFIG_DIR not set (it should be done by the Makefile)"
#endif

typedef struct hm_config_t hm_config_t;

// hm_config_parse reads and parses HM_CONFIG_DIR "/config". Each non-empty,
// non-comment line is of the form `key:val`. If the config file does not exist,
// it is seeded by copying the bundled template at HM_CONFIG_DIR "/default" (the
// default "minimal" configuration) and then parsed. Malformed lines are logged
// and skipped.
// The returned config must be freed with hm_config_free. This never returns NULL.
hm_config_t *hm_config_parse(void);

// hm_config_get returns the value for the first declaration of key, or NULL if
// it is not set. The pointer is valid until hm_config_free is called.
const char *hm_config_get(hm_config_t *cfg, const char *key);

// hm_config_free frees a config returned by hm_config_parse.
void hm_config_free(hm_config_t *cfg);

// hm_global_config_get parses the config on first use (caching it) and returns
// the value for the first declaration of key, or NULL if it is not set. The
// pointer remains valid for the lifetime of the process.
const char *hm_global_config_get(const char *key);

#ifdef __cplusplus
}
#endif
#endif
