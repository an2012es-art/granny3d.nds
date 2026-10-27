
ifeq ($(strip $(DEVKITARM)),)
$(error Please set DEVKITARM)
endif

DEVKITPRO ?= /opt/devkitpro
LIBNDS := $(DEVKITPRO)/libnds
PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
TARGET := granny3d

CFLAGS := -O2 -Wall -std=gnu11 -march=armv5te -mtune=arm946e-s -fomit-frame-pointer -ffast-math -mthumb -mthumb-interwork -DARM9 -I$(LIBNDS)/include
LDFLAGS := -specs=ds_arm9.specs -g -mthumb -mthumb-interwork -L$(LIBNDS)/lib

all: $(TARGET).nds

$(TARGET).elf: source/main.c
	$(PREFIX)gcc $(CFLAGS) $< $(LDFLAGS) -lnds9 -lm -o $@

$(TARGET).nds: $(TARGET).elf
	ndstool -c $@ -9 $<

clean:
	rm -f $(TARGET).elf $(TARGET).nds

.PHONY: all clean
