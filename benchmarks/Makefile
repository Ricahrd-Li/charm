-include ../include/conv-mach-opt.mak

# Temporary hack that needs to be removed / replaced
OPTS += $(OPTSATBUILDTIME)

DIRS = \
  ampi \
  charm++ \

# TODO #2467: eliminate the need for this
ifneq ($(BUILD_CUDA),1)
  DIRS += converse
endif

FTDIRS = \
  charm++ \

TESTDIRS = $(DIRS)

all: $(foreach i,$(DIRS),build-$i)

test: $(foreach i,$(TESTDIRS),test-$i)

clean: $(foreach i,$(DIRS),clean-$i)
	rm -f TAGS #*#
	rm -f core *~

$(foreach i,$(DIRS),build-$i):
	$(MAKE) -C $(subst build-,,$@) all OPTS='$(OPTS)'

$(foreach i,$(DIRS),test-$i):
	$(MAKE) -C $(subst test-,,$@) test OPTS='$(OPTS)' TESTOPTS='$(TESTOPTS)'

$(foreach i,$(DIRS),clean-$i):
	$(MAKE) -C $(subst clean-,,$@) clean OPTS='$(OPTS)'

test-AMPI: test-ampi
test-charm: test-charm++
