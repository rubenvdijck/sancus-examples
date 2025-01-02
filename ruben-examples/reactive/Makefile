APPLICATION ?= reactive
QUIET ?= 1
# DEVELHELP enabled by default for all tests, set 0 to disable
DEVELHELP ?= 0

BOARD = sancus-msp430
RIOTBASE ?= $(CURDIR)/../..

# add current directory to the include path. Putting it in CFLAGS will make
# it go to the beginning, before the standard includes.
# That way slip_params.h get's included and auto configuration can pick it up.
CFLAGS += -I$(CURDIR)

CFLAGS += -DLOG_LEVEL=3 # [0: none, error, warning, info, debug, 5: all]

USE_MINTIMER = 0
TIMING_TESTS = 0
USE_PERIODIC_EVENTS = 0
USE_MMIO_LED = 0
USE_MMIO_BTN = 0

CFLAGS += -DUSE_MINTIMER=$(USE_MINTIMER)
CFLAGS += -DTIMING_TESTS=$(TIMING_TESTS)
CFLAGS += -DUSE_PERIODIC_EVENTS=$(USE_PERIODIC_EVENTS)
CFLAGS += -DUSE_MMIO_LED=$(USE_MMIO_LED)
CFLAGS += -DUSE_MMIO_BTN=$(USE_MMIO_BTN)

# for timer threads
ifeq ($(USE_MINTIMER), 1)
USEMODULE += auto_init
USEMODULE += periph_timer
USEMODULE += mintimer
endif

# sancus libraries for loading modules
LINKFLAGS += -lsm-control -lloader -ltools --inline-arithmetic --verbose
LINKFLAGS += --sm-config-file sm_config.yaml

include $(RIOTBASE)/Makefile.include
