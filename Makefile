include NickelHook/NickelHook.mk

override PKGCONF  += Qt5Widgets
override LIBRARY  := src/libnickelhome.so
override SOURCES  += src/config.c src/nickelhome.cc
override CFLAGS   += -Wall -Wextra -Werror -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemlib -fvisibility=hidden -fvisibility-inlines-hidden
override KOBOROOT += res/doc:$(HM_CONFIG_DIR)/doc res/default:$(HM_CONFIG_DIR)/default

override SKIPCONFIGURE += strip
strip:
	$(STRIP) --strip-unneeded src/libnickelhome.so
.PHONY: strip

ifeq ($(HM_UNINSTALL_CONFIGDIR),1)
override CPPFLAGS += -DHM_UNINSTALL_CONFIGDIR
endif

ifeq ($(HM_CONFIG_DIR),)
override HM_CONFIG_DIR := /mnt/onboard/.adds/nickelhome
endif

override CPPFLAGS += -DHM_CONFIG_DIR='"$(HM_CONFIG_DIR)"' -DHM_CONFIG_DIR_DISP='"$(patsubst /mnt/onboard/%,KOBOeReader/%,$(HM_CONFIG_DIR))"'

include NickelHook/NickelHook.mk
