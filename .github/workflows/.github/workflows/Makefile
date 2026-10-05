.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO no esta definido")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/libnx/switch_rules

TARGET := mi_sys_play

BUILD := build

SOURCES := source

DATA := data

INCLUDES := include

CONFIG_JSON := config.json

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS := -g -Wall -O2 -ffunction-sections \
	$(ARCH) \
	$(BUILD_CFLAGS)

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS := -g $(ARCH)

LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs \
	-g \
	$(ARCH) \
	-Wl,-Map,$(notdir $*.map)

LIBS := -lnx

LIBDIRS := $(PORTLIBS) $(LIBNX)

export OUTPUT := $(CURDIR)/$(TARGET)

export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
	$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))

CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))

export OFILES_SRC := $(CPPFILES:.cpp=.o) \
	$(CFILES:.c=.o) \
	$(SFILES:.s=.o)

export OFILES := $(OFILES_BIN) $(OFILES_SRC)

export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	-I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)

.PHONY: all clean

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "Limpiando..."
	@rm -fr $(BUILD)
	@rm -f $(TARGET).nsp
	@rm -f $(TARGET).nso
	@rm -f $(TARGET).npdm
	@rm -f $(TARGET).elf

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all

all: $(OUTPUT).nsp

$(OUTPUT).nsp: $(OUTPUT).nso $(OUTPUT).npdm

$(OUTPUT).nso: $(OUTPUT).elf

$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC): $(HFILES_BIN)

%.bin.o %_bin.h: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif
