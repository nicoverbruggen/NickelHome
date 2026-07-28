include NickelHook/NickelHook.mk

override PKGCONF  += Qt5Widgets
override LIBRARY  := src/libnickelhome.so
override SOURCES  += src/config.c src/nickelhome.cc
override CFLAGS   += -Wall -Wextra -Werror -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemlib -fvisibility=hidden -fvisibility-inlines-hidden
override KOBOROOT += res/doc:$(NHM_CONFIG_DIR)/doc res/default:$(NHM_CONFIG_DIR)/default res/uninstall:$(NHM_CONFIG_DIR)/uninstall

override SKIPCONFIGURE += strip
strip:
	$(STRIP) --strip-unneeded src/libnickelhome.so
.PHONY: strip

ifeq ($(NHM_CONFIG_DIR),)
override NHM_CONFIG_DIR := /mnt/onboard/.adds/nickel-home
endif

override CPPFLAGS += -DNHM_CONFIG_DIR='"$(NHM_CONFIG_DIR)"' -DNHM_CONFIG_DIR_DISP='"$(patsubst /mnt/onboard/%,KOBOeReader/%,$(NHM_CONFIG_DIR))"'

include NickelHook/NickelHook.mk
