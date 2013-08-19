ACLOCAL_AMFLAGS = -I m4

# --enable-distcheck is here to make it possible to use
# also --enable-dbus-type=session here
AM_DISTCHECK_CONFIGURE_FLAGS = \
    --enable-debug \
    --enable-gtk-doc \
    --enable-distcheck

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gsignond.pc

dist_sysconf_DATA = gsignond.conf

SUBDIRS = src test docs

EXTRA_DIST = dists tools

valgrind:
	cd test; make valgrind
	
dist-hook:
	@echo
	@echo "==== Run 'make update-online-doc' to update online documentation ===="
	@echo

update-online-doc:
	@tar zxf $(PACKAGE)-$(VERSION).tar.gz && \
	pushd . > /dev/null && \
	cd .. && \
	git clone https://code.google.com/p/accounts-sso.gsignon-docs/ ; \
	cd accounts-sso.gsignon-docs && \
	git pull ; \
	mkdir -p $(PACKAGE) && \
	rm -rf $(PACKAGE)/* && \
	popd > /dev/null && \
	cp -rf $(PACKAGE)-$(VERSION)/docs/html/* ../accounts-sso.gsignon-docs/$(PACKAGE)/ && \
	pushd . > /dev/null && \
	cd ../accounts-sso.gsignon-docs && \
	git add $(PACKAGE)/* && \
	git commit -m "Updated documentation for $(PACKAGE) to version $(VERSION)" && \
	git push ; \
	popd > /dev/null && \
	rm -rf $(PACKAGE)-$(VERSION)
