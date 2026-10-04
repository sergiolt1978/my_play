ifeq ($(strip $(DEVKITPRO)),)
$(error "Por favor, establece la variable DEVKITPRO en tu entorno.")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET		:=	mi_sys_play
BUILD		:=	build
SOURCES		:=	.

ARCH		:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -D__SWITCH__ \
			$(INCLUDE)

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	:=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-lnx

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export OUTPUT	:=	$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDES	:=	-I$(CURDIR) -I$(DEVKITPRO)/libnx/include

.PHONY: all clean

all: $(TARGET).nsp

$(TARGET).nsp: $(TARGET).elf config.json
	@elf2nso $(TARGET).elf $(TARGET).nso
	@buildpfs0 $(TARGET).nso config.json $(TARGET).nsp
	@echo "¡Compilado con éxito!"

$(TARGET).elf: $(OFILES)
	@echo Vinculando...
	@$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

%.o: %.cpp
	@echo Compilando $<...
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@rm -f *.o *.elf *.nso *.nsp *.map
