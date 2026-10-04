ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO no está definido")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET  := mi_sys_play
BUILD   := build
SOURCES := .

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS := -g -Wall -O2 -ffunction-sections \
          $(ARCH) \
          -D__SWITCH__

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS := -g $(ARCH)

LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs \
           $(ARCH) \
           -Wl,-Map,$(TARGET).map

LIBS := -lnx

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export OUTPUT := $(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

export OFILES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDES := -I$(CURDIR) -I$(DEVKITPRO)/libnx/include

.PHONY: all clean

all: $(TARGET).elf

$(TARGET).elf: $(OFILES)
	@echo "==> Enlazando $(TARGET).elf"
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.cpp
	@echo "==> Compilando $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	@echo "==> Compilando $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD)
	rm -f *.o *.elf *.map
