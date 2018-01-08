include .fg/_defaults_
project_defaults = .fg/$(PROJECT)/_defaults_
include $(project_defaults)

project_config = .fg/$(PROJECT)/$(CONFIG)/_config_
include $(project_config)

#supress project PRE/POST build steps before include target config
PREBUILD=
POSTBUILD=

target_config=.fg/$(PROJECT)/$(CONFIG)/$(basename $(TARGET))/_config_
-include $(target_config)

#read target groups
groups_file = .fg/$(PROJECT)/$(CONFIG)/$(basename $(TARGET))/_groups_
groups = $(shell cat $(groups_file))
target_name=$(basename $(TARGET))
target_dir=$(OUTDIR)/$(PROJECT)/$(CONFIG)/$(target_name)

# linking target
groups_out = $(addprefix $(target_dir)/, $(addsuffix $(OUTSUFFIX), $(groups)))
$(target_dir)/$(TARGET): $(groups_out)
	@echo ' ' linking [$@]
	$(LINKER) $(LINKER_FLAGS) $^ $(LINKER_OUTFLAG) $@
	$(POSTBUILD)

#read source files from group file
group_file=.fg/$(PROJECT)/$(CONFIG)/$(basename $(TARGET))/$(GROUP)
src=$(shell cat $(group_file))

$(groups_out): %$(OUTSUFFIX): $(target_dir)
	@echo '  ' build group [$(basename $(notdir $@))]
	make $(MAKE_FLAGS) -f $(BUILD_GROUP) PROJECT=$(PROJECT) TARGET=$(TARGET) CONFIG=$(CONFIG) GROUP=$(basename $(notdir $@))

$(target_dir):
	mkdir -p $@

clean:
	for i in $(groups); do \
		make $(MAKE_FLAGS) -f $(BUILD_GROUP)  PROJECT=$(PROJECT) CONFIG=$(CONFIG) TARGET=$(TARGET) GROUP=$$i clean; \
	done
	rm -f $(target_dir)/$(TARGET)
	rm -rf $(target_dir)

