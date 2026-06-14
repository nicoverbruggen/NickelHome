#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

hm_config_t *hm_config_parse(void) {
    hm_config_t *cfg = calloc(1, sizeof(hm_config_t));
    if (!cfg)
        return NULL;

    FILE *f = fopen(HM_CONFIG_DIR "/config", "r");
    if (!f) {
        HM_LOG("no config file at %s/config; no home-screen tweaks will be applied", HM_CONFIG_DIR_DISP);
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
