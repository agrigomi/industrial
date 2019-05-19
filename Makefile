include .fg/_config_

ifeq ($(CONFIG),)
	CONFIG=$(DEFAULT_CONFIG)
endif

ifeq ($(CONFIG),)
	exit
endif

-include $(PROJECT)/.fg/$(CONFIG)/_config_
projects=$(shell cat .fg/projects-$(CONFIG))


.PHONY: all
all: $(projects)
.PHONY: $(projects)


$(projects):
	@echo project [$@] $(CONFIG); \
	if [ -f .fg/$@/build_$@.mk ]; then \
		make $(MAKE_FLAGS) -f .fg/$@/build_$@.mk PROJECT=$@ CONFIG=$(CONFIG); \
	else \
		make $(MAKE_FLAGS) -f $(BUILD_PROJECT) PROJECT=$@ CONFIG=$(CONFIG); \
	fi;

install:
	@for i in $(projects); do \
		echo install [$$i]; \
		make $(MAKE_FLAGS) -f $(INSTALL_PROJECT) PROJECT=$$i CONFIG=$(CONFIG); \
	done


clean:
	@for i in $(projects); do \
		echo clean [$$i] $(CONFIG); \
		if [ -f .fg/$$i/build_$$i.mk ]; then \
			make $(MAKE_FLAGS) -f .fg/$$i/build_$$i.mk PROJECT=$$i CONFIG=$(CONFIG) clean || exit; \
		else \
			make $(MAKE_FLAGS) -f $(BUILD_PROJECT) PROJECT=$$i CONFIG=$(CONFIG) clean || exit; \
		fi; \
	done
