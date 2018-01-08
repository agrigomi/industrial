include .fg/_defaults_
include .fg/$(PROJECT)/_defaults_

project_config=.fg/$(PROJECT)/$(CONFIG)/_config_
-include $(project_config)

target_config=.fg/$(PROJECT)/$(CONFIG)/$(basename $(TARGET))/_config_
-include $(target_config)

group_config=.fg/$(PROJECT)/$(CONFIG)/$(basename $(TARGET))/$(GROUP)._config_
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
	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $< $(PREPROCESSOR_OUTFLAG) $(OUT) > $@
#	$(PREPROCESSOR) $(PREPROCESSOR_FLAGS) $< $(PREPROCESSOR_OUTFLAG) $(@:$(DEPSUFFIX)=$(OUTSUFFIX)) > $@

