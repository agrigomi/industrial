include .fg/_config_
-include $(PROJECT)/.fg/_config_

project_config=$(PROJECT)/.fg/$(CONFIG)/_config_
-include $(project_config)

target_config=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/_config_
-include $(target_config)

group_config=$(PROJECT)/.fg/$(CONFIG)/$(basename $(TARGET))/$(GROUP)._config_
-include $(group_config)

dep_pcfg=
ifneq ("$(wildcard $(project_config))","")
	dep_pcfg=$(project_config)
endif
dep_tcfg=
ifneq ("$(wildcard $(target_config))","")
	dep_tcfg=$(target_config)
endif
dep_gcfg=
ifneq ("$(wildcard $(group_config))","")
	dep_gcfg=$(group_config)
endif

target_name=$(basename $(TARGET))
target_dir=$(OUTDIR)/$(PROJECT)/$(CONFIG)/$(target_name)
OUT=$(addprefix $(target_dir)/$(GROUP)-, $(notdir $(SRC:$(SRCSUFFIX)=$(OUTSUFFIX))))
DEP = $(OUT:$(OUTSUFFIX)=$(DEPSUFFIX))

$(DEP): $(SRC)
	$(PREPROCESSOR) $(DEPENDENCY_FLAGS) $< $(DEPENDENCY_OUTFLAG) $(OUT) > $@

