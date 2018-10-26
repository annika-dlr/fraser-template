# Copyright (c) 2017, German Aerospace Center (DLR)
#
# This file is part of the development version of FRASER.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Authors:
# - 2017, Annika Ofenloch (DLR RY-AVS)

PROG = systemc_adapter
SRCS := $(wildcard *.cpp) \
        $(wildcard ../../fraser/src/communication/*.cpp) \
        $(wildcard ../../../cpp/utils/*.cpp) \
        $(wildcard ../../../cpp/router/*.cpp) \
        $(wildcard ../../../cpp/traffic_generator/*.cpp)

BINDIR = build/bin
OBJDIR = build/obj

## Add 'PTHREADS=1' to command line for a pthreads build
## (should not be needed in most cases)
ifdef PTHREADS
SYSTEMC_CXXFLAGS += -pthread
SYSTEMC_LIBS     += -lpthread
endif

## Set to your local SystemC installation path
ifdef SYS_C_PATH
  SYSTEMC_HOME = $(SYS_C_PATH)
else
  SYSTEMC_HOME = ~/systemc/systemc-2.3.2
endif

## Select the TARGET_ARCH
TARGET_ARCH      = linux64

## default values for additional setup variables
ifneq (,$(strip $(TARGET_ARCH)))
ARCH_SUFFIX      ?= -$(TARGET_ARCH)
endif
LDFLAG_RPATH     ?= -Wl,-rpath=

SYSTEMC_INC_DIR  ?= -I$(SYSTEMC_HOME)/include
SYSTEMC_LIB_DIR  ?= $(SYSTEMC_HOME)/lib$(ARCH_SUFFIX)

SYSTEMC_CXXFLAGS ?= $(FLAGS_COMMON) $(FLAGS_STRICT) $(FLAGS_WERROR)
SYSTEMC_LDFLAGS  ?= -L $(SYSTEMC_LIB_DIR) \
                    $(LDFLAG_RPATH)$(SYSTEMC_LIB_DIR)
SYSTEMC_LIBS     ?= -lsystemc -lm

include ../../makefile.default.mk