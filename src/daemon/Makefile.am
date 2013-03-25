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
                    --identifier-prefix gsignond \
                    --ftail "\nG_END_DECLS\n\n#endif\n" \
                    gsignond-identity.h > $@

gsignond-identity-enum-types.c: $(top_srcdir)/src/common/gsignond-enum.c.template gsignond-identity.h
	$(GLIB_MKENUMS) --template $(top_srcdir)/src/common/gsignond-enum.c.template \
                    --fhead "#include \"gsignond-identity-enum-types.h\"\n" \
                    --identifier-prefix gsignond \
                    gsignond-identity.h > $@

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
    $(BUILT_SOURCES) \
    $(NULL)

gsignond_CFLAGS = \
    -I$(top_srcdir) \
    -I$(top_builddir)/include/ \
    -I$(top_builddir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

gsignond_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/db/libgsignond-db.la \
    $(top_builddir)/src/daemon/dbus/libgsignond-dbus.la \
    $(top_builddir)/src/daemon/plugins/libgsignond-plugins.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

