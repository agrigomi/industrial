include .fg/_defaults_
projects=$(shell cat .fg/projects)


.PHONY: all
all: $(projects)
.PHONY: $(projects)


$(projects):
	@echo build project [$@]; \
	if [ -f .fg/$@/build_$@.mk ]; then \
		make --no-print-directory -f .fg/$@/build_$@.mk PROJECT=$@; \
	else \
		make --no-print-directory -f .fg/build_project.mk PROJECT=$@; \
	fi;

clean:
	for i in $(projects); do \
		echo clean project [$$i]; \
		if [ -f .fg/$$i/build_$$i.mk ]; then \
			make --no-print-directory -f .fg/$$i/build_$$i.mk PROJECT=$$i clean || exit; \
		else \
			make --no-print-directory -f .fg/build_project.mk PROJECT=$$i clean || exit; \
		fi; \
	done
