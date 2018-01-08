include .fg/_defaults_
include .fg/$(PROJECT)/_defaults_
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
	mkdir -p $(OUTDIR)/$(PROJECT)
	mkdir -p $(OUTDIR)/$(PROJECT)/$(CONFIG)
	$(shell $(PREBUILD))
	for i in $(targets); do \
		echo ' ' build target [$$i]; \
		make $(MAKE_FLAGS) -f $(BUILD_TARGET) PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$$i || exit; \
	done
	$(shell $(POSTBUILD))
clean:
	for i in $(targets); do \
		make $(MAKE_FLAGS) -f $(BUILD_TARGET) PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$$i clean || exit; \
	done
	rm -rf $(OUTDIR)/$(PROJECT)

