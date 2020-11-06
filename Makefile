#
# ExMachina Makefile
#

CONFIG:=debug
PLATFORM:=$(strip $(shell clang -print-target-triple))

CC:=clang
DEFINES:=UNICODE _UNICODE _DEBUG
CFLAGS:=$(foreach X,$(DEFINES),-D$X) -g

SRC:=src
OUTDIR:=build/bin/$(PLATFORM)/$(CONFIG)
TARGET:=$(OUTDIR)/vm.exe

#------------------------------------------------------------------------------

C_FILES:=$(wildcard $(SRC)/*.c)
O_FILES:=$(patsubst $(SRC)/%.c,$(OUTDIR)/%.o,$(C_FILES))
D_FILES:=$(O_FILES:.o=.d)

PRINTLN:=@echo

define slashes
$(subst /,\,$1)
endef

define mkdir
mkdir $(call slashes,$1)
endef

define rmdir
rd /S /Q $(call slashes,$1)
endef

define compile
$(strip $(CC) $(CFLAGS)) -MD -MF $(1:.o=.d) -MT "$@ $(basename $@).d" -c -o $(call slashes,$1) $(call slashes,$2)
endef

define link
$(strip $(CC) $(LDFLAGS)) -v -o $1 $2
endef

.PHONY: all stuff clean

all: $(TARGET)

stuff:
	$(PRINTLN) C_FILES: $(C_FILES)
	$(PRINTLN) O_FILES: $(O_FILES)
	$(PRINTLN) D_FILES: $(D_FILES)

$(TARGET): $(O_FILES)
	$(call link,$@,$^)

$(OUTDIR):
	$(call mkdir,$(OUTDIR))

$(OUTDIR)/%.o: $(SRC)/%.c | $(OUTDIR)
	$(call compile,$@,$?)

clean:
	-$(call rmdir,$(OUTDIR))

-include $(D_FILES)