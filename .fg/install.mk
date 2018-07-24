include .fg/_config_
include $(PROJECT)/.fg/_config_
targets = $(shell cat $(PROJECT)/.fg/$(CONFIG)/_targets_)
targets_basename = $(basename $(targets))

#default output directory
OUTDIR=$(DEFAULT_OUTDIR)

ifeq ($(CONFIG),)
	CONFIG=$(DEFAULT_CONFIG)
endif

ifeq ($(CONFIG),)
	exit
endif

-include $(PROJECT)/.fg/$(CONFIG)/_config_

.PHONY: $(targets_basename) clean

$(targets_basename):
	@for i in $(targets); do \
		echo ' ' install target [$$i]; \
	done

.PHONY: $(PROJECT)

$(PROJECT): $(targets_basename)
	@echo install $@
