include .fg/_config_
projects=$(shell cat .fg/projects)


.PHONY: all
all: $(projects)
.PHONY: $(projects)


$(projects):
	@echo project [$@]; \
	if [ -f .fg/$@/build_$@.mk ]; then \
		make $(MAKE_FLAGS) -f .fg/$@/build_$@.mk PROJECT=$@; \
	else \
		make $(MAKE_FLAGS) -f $(BUILD_PROJECT) PROJECT=$@; \
	fi;

install:
	@for i in $(projects); do \
		echo install [$$i]; \
		make $(MAKE_FLAGS) -f $(INSTALL_PROJECT) PROJECT=$$i; \
	done


clean:
	@for i in $(projects); do \
		echo clean [$$i]; \
		if [ -f .fg/$$i/build_$$i.mk ]; then \
			make $(MAKE_FLAGS) -f .fg/$$i/build_$$i.mk PROJECT=$$i clean || exit; \
		else \
			make $(MAKE_FLAGS) -f $(BUILD_PROJECT) PROJECT=$$i clean || exit; \
		fi; \
	done
