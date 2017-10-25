PHONY: default
default: build-all

include msg.mk

SBLIBS = $(addprefix softbrain-, config scheduler emu)

MODULES = $(SBLIBS)  
CLEAN_MODULES = $(addprefix clean-,$(MODULES))

.PHONY: $(MODULES) $(CLEAN_MODULES)

.PHONY: build-all
build-all: $(MODULES)

.PHONY: clean-all
clean-all: $(CLEAN_MODULES)

SIMPLE = $(SBLIBS)

$(SIMPLE):
	$(MAKE) -C $@ install

$(addprefix clean-,$(SIMPLE)):
	$(MAKE) -C $(patsubst clean-%,%,$@) clean

$(addprefix clean-,$(AUTOTOOLS)):
	rm -rf $(patsubst clean-%,%,$@)/build

# Dependencies
softbrain-scheduler: softbrain-config
softbrain-emu: softbrain-scheduler softbrain-config


full-rebuild:
	@echo "Wipe \$$SS_TOOLS ($$SS_TOOLS) and rebuild everything?"
	@read -p "[Y/n]: " yn && { [ -z $$yn ] || [ $$yn = Y ] || [ $$yn = y ]; }
	rm -rf "$$SS_TOOLS"
	$(MAKE) clean-all
	$(MAKE) build-all

