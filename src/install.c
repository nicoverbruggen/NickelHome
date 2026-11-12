#include <stddef.h>
#include <resources.h>
#include "embedded_resources.h"

const struct nh_resources nh_resources = {
    .config_dir = NHM_CONFIG_DIR,
    .uninstall_file = NHM_CONFIG_DIR "/uninstall-now",
    .files = nh_embedded_resources,
};
