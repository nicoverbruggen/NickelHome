#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "util.h"

typedef struct nhm_config_entry_t {
    char *key;
    char *val;
    struct nhm_config_entry_t *next;
} nhm_config_entry_t;

struct nhm_config_t {
    nhm_config_entry_t *head;
    nhm_config_entry_t *tail;
};

// nhm_config_append adds key/val to the config (taking ownership of copies).
static void nhm_config_append(nhm_config_t *cfg, const char *key, const char *val) {
    nhm_config_entry_t *e = calloc(1, sizeof(nhm_config_entry_t));
    if (!e || !(e->key = strdup(key)) || !(e->val = strdup(val))) {
        NHM_LOG("warning: out of memory while parsing config, skipping '%s'", key);
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

// nhm_config_write_default seeds NHM_CONFIG_DIR "/config" by copying the bundled
// template at NHM_CONFIG_DIR "/default" (installed from res/default), which holds
// the default "minimal" configuration.
static void nhm_config_write_default(void) {
    // the config dir normally exists (the doc/default files are installed there),
    // but create it defensively in case it was removed
    mkdir(NHM_CONFIG_DIR, 0755);

    FILE *src = fopen(NHM_CONFIG_DIR "/default", "r");
    if (!src) {
        NHM_LOG("warning: no default config template at %s/default (%s); leaving config absent", NHM_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    FILE *dst = fopen(NHM_CONFIG_DIR "/config", "w");
    if (!dst) {
        NHM_LOG("warning: could not write default config to %s/config (%s)", NHM_CONFIG_DIR_DISP, strerror(errno));
        fclose(src);
        return;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            NHM_LOG("warning: could not fully write default config to %s/config", NHM_CONFIG_DIR_DISP);
            break;
        }
    }

    fclose(src);
    fclose(dst);
    NHM_LOG("wrote default config to %s/config from template", NHM_CONFIG_DIR_DISP);
}

nhm_config_t *nhm_config_parse(void) {
    nhm_config_t *cfg = calloc(1, sizeof(nhm_config_t));
    if (!cfg)
        return NULL;

    FILE *f = fopen(NHM_CONFIG_DIR "/config", "r");
    if (!f && errno == ENOENT) {
        NHM_LOG("no config file at %s/config; writing a default one", NHM_CONFIG_DIR_DISP);
        nhm_config_write_default();
        f = fopen(NHM_CONFIG_DIR "/config", "r");
    }
    if (!f) {
        NHM_LOG("could not open %s/config (%s); no home-screen tweaks will be applied", NHM_CONFIG_DIR_DISP, strerror(errno));
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
            NHM_LOG("warning: %s/config: line %d: expected key, ignoring line", NHM_CONFIG_DIR_DISP, lineno);
            continue;
        }
        if (!cur) {
            NHM_LOG("warning: %s/config: line %d: expected ':' after key '%s', ignoring line", NHM_CONFIG_DIR_DISP, lineno, key);
            continue;
        }
        char *val = strtrim(cur);

        nhm_config_append(cfg, key, val);
        NHM_LOG("config: %s = %s", key, val);
    }

    free(buf);
    fclose(f);
    return cfg;
}

const char *nhm_config_get(nhm_config_t *cfg, const char *key) {
    if (!cfg)
        return NULL;
    for (nhm_config_entry_t *e = cfg->head; e; e = e->next)
        if (!strcmp(e->key, key))
            return e->val;
    return NULL;
}

void nhm_config_free(nhm_config_t *cfg) {
    if (!cfg)
        return;
    nhm_config_entry_t *e = cfg->head;
    while (e) {
        nhm_config_entry_t *next = e->next;
        free(e->key);
        free(e->val);
        free(e);
        e = next;
    }
    free(cfg);
}

const char *nhm_global_config_get(const char *key) {
    static nhm_config_t *global = NULL;
    if (!global)
        global = nhm_config_parse();
    return nhm_config_get(global, key);
}
