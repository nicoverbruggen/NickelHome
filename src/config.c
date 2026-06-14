#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "util.h"

typedef struct hm_config_entry_t {
    char *key;
    char *val;
    struct hm_config_entry_t *next;
} hm_config_entry_t;

struct hm_config_t {
    hm_config_entry_t *head;
    hm_config_entry_t *tail;
};

// hm_config_append adds key/val to the config (taking ownership of copies).
static void hm_config_append(hm_config_t *cfg, const char *key, const char *val) {
    hm_config_entry_t *e = calloc(1, sizeof(hm_config_entry_t));
    if (!e || !(e->key = strdup(key)) || !(e->val = strdup(val))) {
        HM_LOG("warning: out of memory while parsing config, skipping '%s'", key);
        if (e) {
            free(e->key);
            free(e->val);
            free(e);
        }
        return;
    }
    if (cfg->tail)
        cfg->tail->next = e;
    else
        cfg->head = e;
    cfg->tail = e;
}

// hm_config_write_default seeds HM_CONFIG_DIR "/config" by copying the bundled
// template at HM_CONFIG_DIR "/default" (installed from res/default), which holds
// the default "minimal" configuration.
static void hm_config_write_default(void) {
    // the config dir normally exists (the doc/default files are installed there),
    // but create it defensively in case it was removed
    mkdir(HM_CONFIG_DIR, 0755);

    FILE *src = fopen(HM_CONFIG_DIR "/default", "r");
    if (!src) {
        HM_LOG("warning: no default config template at %s/default (%s); leaving config absent", HM_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    FILE *dst = fopen(HM_CONFIG_DIR "/config", "w");
    if (!dst) {
        HM_LOG("warning: could not write default config to %s/config (%s)", HM_CONFIG_DIR_DISP, strerror(errno));
        fclose(src);
        return;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            HM_LOG("warning: could not fully write default config to %s/config", HM_CONFIG_DIR_DISP);
            break;
        }
    }

    fclose(src);
    fclose(dst);
    HM_LOG("wrote default config to %s/config from template", HM_CONFIG_DIR_DISP);
}

hm_config_t *hm_config_parse(void) {
    hm_config_t *cfg = calloc(1, sizeof(hm_config_t));
    if (!cfg)
        return NULL;

    FILE *f = fopen(HM_CONFIG_DIR "/config", "r");
    if (!f && errno == ENOENT) {
        HM_LOG("no config file at %s/config; writing a default one", HM_CONFIG_DIR_DISP);
        hm_config_write_default();
        f = fopen(HM_CONFIG_DIR "/config", "r");
    }
    if (!f) {
        HM_LOG("could not open %s/config (%s); no home-screen tweaks will be applied", HM_CONFIG_DIR_DISP, strerror(errno));
        return cfg;
    }

    char *buf = NULL;
    size_t bufsz = 0;
    ssize_t len;
    int lineno = 0;
    while ((len = getline(&buf, &bufsz, f)) != -1) {
        lineno++;

        // strip comments
        char *hash = strchr(buf, '#');
        if (hash)
            *hash = '\0';

        char *line = strtrim(buf);
        if (!*line)
            continue;

        char *cur = line;
        char *key = strsep(&cur, ":");
        key = strtrim(key);
        if (!key || !*key) {
            HM_LOG("warning: %s/config: line %d: expected key, ignoring line", HM_CONFIG_DIR_DISP, lineno);
            continue;
        }
        if (!cur) {
            HM_LOG("warning: %s/config: line %d: expected ':' after key '%s', ignoring line", HM_CONFIG_DIR_DISP, lineno, key);
            continue;
        }
        char *val = strtrim(cur);

        hm_config_append(cfg, key, val);
        HM_LOG("config: %s = %s", key, val);
    }

    free(buf);
    fclose(f);
    return cfg;
}

const char *hm_config_get(hm_config_t *cfg, const char *key) {
    if (!cfg)
        return NULL;
    for (hm_config_entry_t *e = cfg->head; e; e = e->next)
        if (!strcmp(e->key, key))
            return e->val;
    return NULL;
}

void hm_config_free(hm_config_t *cfg) {
    if (!cfg)
        return;
    hm_config_entry_t *e = cfg->head;
    while (e) {
        hm_config_entry_t *next = e->next;
        free(e->key);
        free(e->val);
        free(e);
        e = next;
    }
    free(cfg);
}

const char *hm_global_config_get(const char *key) {
    static hm_config_t *global = NULL;
    if (!global)
        global = hm_config_parse();
    return hm_config_get(global, key);
}
