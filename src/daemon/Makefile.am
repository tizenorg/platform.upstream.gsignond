SUBDIRS = db dbus plugins
NULL=

bin_PROGRAMS = gsignond

BUILT_SOURCES = \
    gsignond-identity-enum-types.h \
    gsignond-identity-enum-types.c \
    $(NULL)

gsignond-identity-enum-types.h: $(top_srcdir)/src/common/gsignond-enum.h.template gsignond-identity.h
	$(GLIB_MKENUMS) --template $(top_srcdir)/src/common/gsignond-enum.h.template \
                    --fhead "#ifndef GSIGNOND_IDENTITY_ENUM_TYPES_H_\n#define GSIGNOND_IDENTITY_ENUM_TYPES_H_\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS" \
                    --identifier-prefix GSignond \
                    --ftail "\nG_END_DECLS\n\n#endif\n" \
                    $(srcdir)/gsignond-identity.h > $@

gsignond-identity-enum-types.c: $(top_srcdir)/src/common/gsignond-enum.c.template gsignond-identity.h
	$(GLIB_MKENUMS) --template $(top_srcdir)/src/common/gsignond-enum.c.template \
                    --fhead "#include \"gsignond-identity-enum-types.h\"\n" \
                    --identifier-prefix GSignond \
                    $(srcdir)/gsignond-identity.h > $@

gsignond_includedir = $(includedir)/gsignond
gsignond_pubhdr = ../../include/gsignond
gsignond_include_HEADERS = \
	$(gsignond_pubhdr)/gsignond-access-control-manager.h \
	$(gsignond_pubhdr)/gsignond-config-db.h \
	$(gsignond_pubhdr)/gsignond-config-dbus.h \
	$(gsignond_pubhdr)/gsignond-config-general.h \
	$(gsignond_pubhdr)/gsignond-config.h \
	$(gsignond_pubhdr)/gsignond-credentials.h \
	$(gsignond_pubhdr)/gsignond-dictionary.h \
	$(gsignond_pubhdr)/gsignond-error.h \
	$(gsignond_pubhdr)/gsignond-extension-interface.h \
	$(gsignond_pubhdr)/gsignond-identity-info.h \
	$(gsignond_pubhdr)/gsignond-log.h \
	$(gsignond_pubhdr)/gsignond-plugin-interface.h \
	$(gsignond_pubhdr)/gsignond-secret-storage.h \
	$(gsignond_pubhdr)/gsignond-security-context.h \
	$(gsignond_pubhdr)/gsignond-session-data.h \
	$(gsignond_pubhdr)/gsignond-signonui-data.h \
	$(gsignond_pubhdr)/gsignond-storage-manager.h \
	$(gsignond_pubhdr)/gsignond-utils.h

gsignond_SOURCES = \
    main.c \
    gsignond-auth-session.c \
    gsignond-auth-session.h \
    gsignond-daemon.c \
    gsignond-daemon.h \
    gsignond-identity.c \
    gsignond-identity.h \
    gsignond-signonui-proxy.c \
    gsignond-signonui-proxy.h \
    gsignond-types.h \
    $(BUILT_SOURCES) \
    $(NULL)

gsignond_CPPFLAGS = \
    -I. \
    $(NULL)

gsignond_CFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/include/ \
    -I$(top_srcdir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

gsignond_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/db/libgsignond-db.la \
    $(top_builddir)/src/daemon/dbus/libgsignond-dbus.la \
    $(top_builddir)/src/daemon/plugins/libgsignond-plugins.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

if SET_PERMISSIONS
install-exec-hook:
	chown root $(DESTDIR)$(bindir)/gsignond || true
	chmod u+s $(DESTDIR)$(bindir)/gsignond || true
endif

CLEANFILES = \
    gsignond-identity-enum-types.h \
    gsignond-identity-enum-types.c \
    $(NULL)
