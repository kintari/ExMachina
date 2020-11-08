
PLATFORM:=x86_64-windows-msvc
CC:=cl.exe
DEFINES:=UNICODE _UNICODE _DEBUG
EXE_SUFFIX:=.exe
OBJ_SUFFIX:=obj
CFLAGS=/GA /TC /FC

ifeq ($(CONFIG),Debug)
	CFLAGS+=/Od /Zi /MTd
	LDFLAGS+=/DEBUG
endif
ifeq ($(CONFIG),Release)
	CFLAGS+=/O2 /Oi /MT
endif

define path
$(subst /,\,$1)
endef

define compile
$(strip $(CC) $(CFLAGS) $(foreach X,$(DEFINES),-D$X)) /c $2 /Fd:$(call path,"$(1:.obj=.pdb)") /Fo:$(call path,"$1")
endef

define link
$(strip $(CC) /nologo $(LDFLAGS)) -Fe:"$(call path,$1)" $(foreach X,$2,"$(call path,$X)")
endef

define mkdir
mkdir $(call path,$1)
endef

define rmdir
rd /S /Q $(call path,$1)
endef

#/Fe:"$(call slashes,$1)"