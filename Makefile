#
# ExMachina Makefile
#

CONFIG:=Debug

ifeq ($(OS),Windows_NT)
include mk/Windows.mk
endif
ifeq ($(OS),Linux)
include mk/Linux.mk
endif

SRC:=src
OUTDIR:=build/bin/$(PLATFORM)/$(CONFIG)
TARGET:=$(OUTDIR)/exmc

#------------------------------------------------------------------------------

C_FILES:=$(wildcard $(SRC)/*.c)
H_FILES:=$(wildcard $(SRC)/*.h)

OBJECTS:=$(patsubst $(SRC)/%.c,$(OUTDIR)/%.$(OBJ_SUFFIX),$(C_FILES))

.PHONY: all clean build run

all: rebuild

build: $(TARGET)

rebuild: clean $(TARGET)

clean:
	-$(call rmdir,$(OUTDIR))

run:
	$(call path,./$(TARGET))

$(TARGET): $(OBJECTS)
	$(call link,$@,$^)

$(OUTDIR):
	$(call mkdir,$(OUTDIR))

$(OUTDIR)/%.$(OBJ_SUFFIX): $(SRC)/%.c | $(OUTDIR)
	$(call compile,$@,$?)

