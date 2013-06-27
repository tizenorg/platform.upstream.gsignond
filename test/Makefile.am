if HAVE_DEBUG
SUBDIRS = common daemon db plugins
else
SUBDIRS =

check-local:
	@echo "ERROR: tests are enabled only if ./configure is run with --enable-debug"
	@exit 1
endif

VALGRIND_TESTS_DISABLE =
valgrind: $(SUBDIRS)
	for t in $(filter-out $(VALGRIND_TESTS_DISABLE),$(SUBDIRS)); do \
		cd $$t; $(MAKE) valgrind; cd ..;\
	done;

EXTRA_DIST = \
    valgrind.supp \
    valgrind_common.mk	
