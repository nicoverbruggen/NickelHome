NH_QT_MAJOR ?= 5
NICKELHOOK ?= NickelHook/
include $(patsubst %/,%,$(NICKELHOOK))/NickelHook.mk

override PKGCONF  += Qt$(NH_QT_MAJOR)Widgets
override LIBRARY  := src/libnickelhome.so
override SOURCES  += src/config.c
ifeq ($(NH_QT_MAJOR),6)
override SOURCES += src/home-qt6.cc src/findbooks.cc src/install.c $(patsubst %/,%,$(NICKELHOOK))/resources.c
else
override SOURCES += src/nickelhome.cc
endif
override CFLAGS   += -Wall -Wextra -Werror -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemlib -fvisibility=hidden -fvisibility-inlines-hidden
ifneq ($(NH_QT_MAJOR),6)
override KOBOROOT += res/doc:$(NHM_CONFIG_DIR)/doc res/default:$(NHM_CONFIG_DIR)/default res/uninstall:$(NHM_CONFIG_DIR)/uninstall
endif

override SKIPCONFIGURE += strip
strip:
	$(STRIP) --strip-unneeded src/libnickelhome.so
.PHONY: strip

ifeq ($(NHM_CONFIG_DIR),)
override NHM_CONFIG_DIR := /mnt/onboard/.adds/nickel-home
endif

override CPPFLAGS += -DNHM_CONFIG_DIR='"$(NHM_CONFIG_DIR)"' -DNHM_CONFIG_DIR_DISP='"$(patsubst /mnt/onboard/%,KOBOeReader/%,$(NHM_CONFIG_DIR))"'

include $(patsubst %/,%,$(NICKELHOOK))/NickelHook.mk

ifeq ($(NH_QT_MAJOR),6)
override GENERATED += src/embedded_resources.h
src/install.o: src/embedded_resources.h
src/embedded_resources.h: res/doc res/default res/uninstall $(patsubst %/,%,$(NICKELHOOK))/embed_resources.py
	python3 $(patsubst %/,%,$(NICKELHOOK))/embed_resources.py $@ res/doc res/default res/uninstall
endif
