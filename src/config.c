#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

// Set on any config problem (unknown key, malformed line, invalid value); read by NHM_DBG so a
// broken config turns verbose logging on for the whole boot and diagnoses itself in the log.
static bool nhm_config_problem = false;
bool nhm_config_problem_seen(void) {
    return nhm_config_problem;
}

// Cached nhm_log:1 for NHM_DBG, published at the end of the parse. The logger can't ask the
// global config directly: the global config is built by this parser, and logging mid-parse would
// recurse into it.
bool nhm_log_verbose = false;

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

// nhm_config_write_default seeds NHM_CONFIG_DIR "/config" by copying the bundled template at
// NHM_CONFIG_DIR "/default" (installed from res/default), which holds the default "minimal"
// configuration. The copy is atomic (write a unique sibling, flush, rename) so a power cut can't
// leave a truncated config behind.
static void nhm_config_write_default(void) {
    // the config dir normally exists (the doc/default files are installed there),
    // but create it defensively in case it was removed
    if (mkdir(NHM_CONFIG_DIR, 0755) != 0 && errno != EEXIST) {
        NHM_LOG("warning: could not create %s (%s)", NHM_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    FILE *src = fopen(NHM_CONFIG_DIR "/default", "r");
    if (!src) {
        NHM_LOG("warning: no default config template at %s/default (%s); leaving config absent", NHM_CONFIG_DIR_DISP, strerror(errno));
        return;
    }

    char tmp[1024];
    int n = snprintf(tmp, sizeof(tmp), NHM_CONFIG_DIR "/config.tmp.%ld", (long)getpid());
    if (n < 0 || (size_t)n >= sizeof(tmp)) {
        NHM_LOG("warning: default config path is too long");
        fclose(src);
        return;
    }
    int fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    if (fd < 0) {
        NHM_LOG("warning: could not write default config to %s/config (%s)", NHM_CONFIG_DIR_DISP, strerror(errno));
        fclose(src);
        return;
    }
    FILE *dst = fdopen(fd, "w");
    if (!dst) {
        int saved_errno = errno;
        close(fd);
        unlink(tmp);
        fclose(src);
        NHM_LOG("warning: could not write default config to %s/config (%s)", NHM_CONFIG_DIR_DISP, strerror(saved_errno));
        return;
    }

    bool ok = true;
    char buf[4096];
    size_t got;
    while ((got = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, got, dst) != got) {
            ok = false;
            break;
        }
    }
    if (ferror(src)) ok = false;
    fclose(src);
    if (ok && fflush(dst) != 0) ok = false;
    if (ok && fsync(fileno(dst)) != 0) ok = false;
    if (fclose(dst) != 0) ok = false;
    if (!ok) {
        NHM_LOG("warning: could not flush default config to %s/config (%s)", NHM_CONFIG_DIR_DISP, strerror(errno));
        unlink(tmp);
        return;
    }
    if (rename(tmp, NHM_CONFIG_DIR "/config") != 0) {
        NHM_LOG("warning: could not install default config at %s/config (%s)", NHM_CONFIG_DIR_DISP, strerror(errno));
        unlink(tmp);
        return;
    }
    int dir_fd = open(NHM_CONFIG_DIR, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dir_fd >= 0) {
        fsync(dir_fd);
        close(dir_fd);
    }
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
            nhm_config_problem = true;
            NHM_LOG("warning: %s/config: line %d: expected key, ignoring line", NHM_CONFIG_DIR_DISP, lineno);
            continue;
        }
        if (!cur) {
            nhm_config_problem = true;
            NHM_LOG("warning: %s/config: line %d: expected ':' after key '%s', ignoring line", NHM_CONFIG_DIR_DISP, lineno, key);
            continue;
        }

        bool known = false;
        for (size_t i = 0; nhm_known_keys[i]; i++)
            if (!strcmp(key, nhm_known_keys[i])) { known = true; break; }
        if (!known) {
            nhm_config_problem = true;
            NHM_LOG("warning: %s/config: line %d: unknown setting '%s' (it does nothing — likely a typo; the doc file lists the valid settings)", NHM_CONFIG_DIR_DISP, lineno, key);
        }

        char *val = strtrim(cur);
        nhm_config_append(cfg, key, val);
    }

    free(buf);
    fclose(f);

    // Publish verbosity for NHM_DBG, then echo the parsed keys under verbose logging — or when
    // the config has a problem, so a broken config always shows what was actually parsed. A
    // healthy boot with nhm_log:0 echoes nothing.
    nhm_log_verbose = nhm_config_bool(cfg, "nhm_log", false);
    if (nhm_log_verbose || nhm_config_problem)
        for (nhm_config_entry_t *e = cfg->head; e; e = e->next)
            NHM_LOG("config: %s = %s", e->key, e->val);
    return cfg;
}

const char *nhm_config_get(nhm_config_t *cfg, const char *key) {
    if (!cfg)
        return NULL;
    // first declaration of a key wins (the list preserves file order)
    for (nhm_config_entry_t *e = cfg->head; e; e = e->next)
        if (!strcmp(e->key, key))
            return e->val;
    return NULL;
}

bool nhm_config_bool(nhm_config_t *cfg, const char *key, bool default_value) {
    const char *val = nhm_config_get(cfg, key);
    if (!val || !*val)
        return default_value;
    if (!strcmp(val, "1") || !strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on"))
        return true;
    if (!strcmp(val, "0") || !strcasecmp(val, "false") || !strcasecmp(val, "no") || !strcasecmp(val, "off"))
        return false;

    nhm_config_problem = true;
    NHM_LOG("warning: invalid boolean for '%s': '%s'; using default %d", key, val, default_value ? 1 : 0);
    return default_value;
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

static nhm_config_t *nhm_global_config(void) {
    static nhm_config_t *global = NULL;
    if (!global)
        global = nhm_config_parse();
    return global;
}

const char *nhm_global_config_get(const char *key) {
    return nhm_config_get(nhm_global_config(), key);
}

bool nhm_global_config_bool(const char *key, bool default_value) {
    return nhm_config_bool(nhm_global_config(), key, default_value);
}
