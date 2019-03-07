UK_ROOT ?= $(PWD)/../../staging
UK_LIBS ?= $(PWD)/../../libs
LIBS := $(UK_LIBS)/newlib
all:
	@make -C $(UK_ROOT) A=$(PWD) L=$(LIBS)

$(MAKECMDGOALS):
	@make -C $(UK_ROOT) A=$(PWD) L=$(LIBS) $(MAKECMDGOALS)
