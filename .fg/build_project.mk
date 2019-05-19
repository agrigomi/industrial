include .fg/_config_
-include $(PROJECT)/.fg/_config_
targets = $(shell cat $(PROJECT)/.fg/$(CONFIG)/_targets_)
targets_basename = $(basename $(targets))

#default output directory
OUTDIR=$(DEFAULT_OUTDIR)

.PHONY: $(targets_basename) clean

$(targets_basename):
	@mkdir -p $(OUTDIR)/$(PROJECT)
	@mkdir -p $(OUTDIR)/$(PROJECT)/$(CONFIG)
	@for i in $(targets); do \
		echo ' ' target [$$i]; \
		make $(MAKE_FLAGS) -f $(BUILD_TARGET) PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$$i || exit; \
	done
clean:
	@for i in $(targets); do \
		make $(MAKE_FLAGS) -f $(BUILD_TARGET) PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$$i clean || exit; \
	done
	rm -rf $(OUTDIR)/$(PROJECT)

