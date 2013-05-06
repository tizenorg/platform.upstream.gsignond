SUBDIRS = common daemon db plugins

VALGRIND_TESTS_DISABLE =
valgrind: $(SUBDIRS)
	for t in $(filter-out $(VALGRIND_TESTS_DISABLE),$(SUBDIRS)); do \
		cd $$t; $(MAKE) valgrind; cd ..;\
	done;
