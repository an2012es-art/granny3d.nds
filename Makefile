ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM (install devkitPro)")
endif

DEVKITPRO ?= /opt/devkitpro
LIBNDS    := $(DEVKITPRO)/libnds
PREFIX    := $(DEVKITARM)/bin/arm-none-eabi-
TARGET    := granny3d

ARCH    := -mthumb -mthumb-interwork
CFLAGS  := -O2 -Wall -march=armv5te -mtune=arm946e-s -fomit-frame-pointer \
           -ffast-math $(ARCH) -DARM9 -I$(LIBNDS)/include
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -L$(LIBNDS)/lib

ICON := $(wildcard $(LIBNDS)/icon.bmp)
ifneq ($(ICON),)
BANNER := -b $(ICON) "Haunted House 3D;Horror game;DS"
endif

all: $(TARGET).nds

$(TARGET).elf: source/main.c
	$(PREFIX)gcc $(CFLAGS) source/main.c $(LDFLAGS) -lnds9 -lm -o $@

$(TARGET).nds: $(TARGET).elf
	ndstool -c $@ -9 $< $(BANNER)

clean:
	rm -f $(TARGET).elf $(TARGET).nds

.PHONY: all clean
