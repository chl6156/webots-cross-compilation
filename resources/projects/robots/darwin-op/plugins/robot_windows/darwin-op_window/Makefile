ifeq ($(OS),Windows_NT)
 OSTYPE = windows
endif

ifndef OSTYPE
 OSTYPE = $(shell uname)
endif

ifeq ($(OSTYPE),Linux)
 OSTYPE = linux
endif

ifeq ($(OSTYPE),linux-gnu)
 OSTYPE = linux
endif

ifeq ($(OSTYPE),)
 OSTYPE = windows
endif

ifeq ($(OSTYPE),msys)
 OSTYPE = windows
endif

ifneq ($(findstring MINGW,$(OSTYPE)),)
 OSTYPE = windows
endif

ifneq ($(findstring CYGWIN,$(OSTYPE)),)
 OSTYPE = windows
endif

ifeq ($(OSTYPE),Darwin)
 OSTYPE = darwin
endif

CXX_SOURCES = entry_points.cpp Viewer.cpp Transfer.cpp SSH.cpp ZIP.cpp 
HPP_FILES_TO_MOC = Transfer.hpp SSH.hpp

QT = core gui widgets concurrent
USE_C_API = true

QT_UTILS = $(WEBOTS_HOME)/resources/projects/default/libraries/qt_utils
INCLUDE = -I"$(QT_UTILS)" -I"$(WEBOTS_HOME)/include/libssh" -I"$(WEBOTS_HOME)/include/libzip"
LIBRARIES = -L"$(QT_UTILS)" -lqt_utils -L"$(WEBOTS_HOME)/lib" -lssh

ifeq ($(OSTYPE),windows)
LIBRARIES += -lpthread -lzip-2
endif

ifeq ($(OSTYPE),darwin)
LIBRARIES += -lzip.2
endif

ifeq ($(OSTYPE),linux)
LIBRARIES += -lzip
endif

space :=
space +=
WEBOTS_HOME_PATH=$(subst $(space),\ ,$(strip $(subst \,/,$(WEBOTS_HOME))))
include $(WEBOTS_HOME_PATH)/resources/Makefile.include
