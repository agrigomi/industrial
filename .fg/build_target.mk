include .fg/_config_
project_defaults = $(PROJECT)/.fg/_config_
-include $(project_defaults)

project_config = $(PROJECT)/.fg/$(CONFIG)/_config_
include $(project_config)

#supress project PRE/POST build steps before include target config

target_config=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/_config_
-include $(target_config)

#read target groups
groups_file = $(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/_groups_
groups = $(shell cat $(groups_file))
target_name=$(basename $(TARGET))
target_dir=$(OUTDIR)/$(PROJECT)/$(CONFIG)/$(target_name)

# linking target
groups_out = $(addprefix $(target_dir)/, $(addsuffix $(OUTSUFFIX), $(groups)))
$(target_dir)/$(TARGET): $(groups_out)
	@echo ' ' linking [$@]
	$(LINK_TARGET)
	$(POSTBUILD)

#read source files from group file
group_file=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/$(GROUP)
src=$(shell cat $(group_file))

$(groups_out): %$(OUTSUFFIX): $(target_dir)
	@echo '  ' group [$(basename $(notdir $@))]
	make $(MAKE_FLAGS) -f $(BUILD_GROUP) PROJECT=$(PROJECT) TARGET=$(TARGET) CONFIG=$(CONFIG) GROUP=$(basename $(notdir $@))

$(target_dir):
	mkdir -p $@

clean:
	@for i in $(groups); do \
		make $(MAKE_FLAGS) -f $(BUILD_GROUP)  PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$(TARGET) GROUP=$$i clean; \
	done
	@rm -f $(target_dir)/$(TARGET)
	@rm -rf $(target_dir)
	@rm -f $(DEPLOY_DIR)/$(TARGET)

