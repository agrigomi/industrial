include .fg/_config_
include .fg/$(PROJECT)/_config_
targets = $(shell cat .fg/$(PROJECT)/$(CONFIG)/_targets_)
targets_basename = $(basename $(targets))

#default output directory
OUTDIR=$(DEFAULT_OUTDIR)

ifeq ($(CONFIG),)
	CONFIG=$(DEFAULT_CONFIG)
endif

ifeq ($(CONFIG),)
	exit
endif

-include .fg/$(PROJECT)/$(CONFIG)/_config_

.PHONY: $(targets_basename) clean

$(targets_basename):
	@for i in $(targets); do \
		echo ' ' install target [$$i]; \
	done

.PHONY: $(PROJECT)

$(PROJECT): $(targets_basename)
	@echo install $@
