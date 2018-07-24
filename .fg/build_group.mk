include .fg/_config_
-include $(PROJECT)/.fg/_config_

project_config=$(PROJECT)/.fg/$(CONFIG)/_config_
-include $(project_config)

target_config=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/_config_
-include $(target_config)

#supress target PRE/POST build steps before include group config
#set default linker flag for link relocatable
LINKER=ld
LINKER_FLAGS=-r
group_config=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/$(GROUP)._config_
-include $(group_config)

#read source files from group file
group_file=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/$(GROUP)

target_name=$(basename $(TARGET))
target_dir=$(OUTDIR)/$(PROJECT)/$(CONFIG)/$(target_name)
group_target=$(target_dir)/$(GROUP)$(OUTSUFFIX)
#read group sources
group_src=$(shell cat $(group_file))
group_out=$(addprefix $(target_dir)/$(GROUP)-, $(notdir $(group_src:$(SRCSUFFIX)=$(OUTSUFFIX))))
group_dep=$(addprefix $(target_dir)/$(GROUP)-, $(notdir $(group_src:$(SRCSUFFIX)=$(DEPSUFFIX))))

.PHONY: clean $(group_dep)

$(group_target): $(group_out)
	@echo '    ' [$(LINKER)] $@
	$(LINKER) $(LINKER_FLAGS) $^ $(LINKER_OUTFLAG) $@

$(group_out): $(group_dep)
	@for i in $(group_src); do \
		make $(MAKE_FLAGS) -f $(BUILD_FILE) PROJECT=$(PROJECT) TARGET=$(TARGET) CONFIG=$(CONFIG) GROUP=$(GROUP) SRC=$$i || exit; \
	done

$(group_dep): $(group_src)
	@for i in $(group_src); do \
		make $(MAKE_FLAGS) -f $(BUILD_DEPENDENCY) PROJECT=$(PROJECT) TARGET=$(TARGET) CONFIG=$(CONFIG) GROUP=$(GROUP) SRC=$$i || exit; \
	done

clean:
	rm -f $(group_out) $(group_target)

