include $(top_srcdir)/common.mk

SUBDIRS = db 
NULL=

lib_LTLIBRARIES = libgsignond-common.la

BUILT_SOURCES = \
    gsignond-plugin-enum-types.h \
    gsignond-plugin-enum-types.c \
    $(NULL)

GSIGNOND_INCLUDE_DIR=$(top_srcdir)/include/gsignond

gsignond-plugin-enum-types.h: gsignond-enum.h.template $(GSIGNOND_INCLUDE_DIR)/gsignond-plugin-interface.h
	$(GLIB_MKENUMS) --template $(srcdir)/gsignond-enum.h.template \
                    --fhead "#ifndef GSIGNOND_PLUGIN_ENUM_TYPES_H_\n#define GSIGNOND_PLUGIN_ENUM_TYPES_H_\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS" \
                    --identifier-prefix gsignond \
                    --ftail "\nG_END_DECLS\n\n#endif\n" \
                    $(GSIGNOND_INCLUDE_DIR)/gsignond-plugin-interface.h > $@

gsignond-plugin-enum-types.c: gsignond-enum.c.template $(GSIGNOND_INCLUDE_DIR)/gsignond-plugin-interface.h
	$(GLIB_MKENUMS) --template $(srcdir)/gsignond-enum.c.template \
                    --fhead "#include \"gsignond-plugin-enum-types.h\"\n" \
                    --identifier-prefix gsignond \
                    $(GSIGNOND_INCLUDE_DIR)/gsignond-plugin-interface.h > $@


libgsignond_common_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    -I. \
    $(GSIGNOND_CFLAGS) \
    -DGSIGNOND_PLUGINS_DIR='"$(pluginsdir)"' \
    -DGSIGNOND_EXTENSIONS_DIR='"$(extensionsdir)"' \
    -DGSIGNOND_BIN_DIR='"$(bindir)"' \
    -DGSIGNOND_SYSCONF_DIR='"$(sysconfdir)"' \
    $(NULL)

libgsignond_common_la_DEPENDENCIES = \
    $(top_builddir)/src/common/db/libgsignond-common-db.la \
    $(NULL)

libgsignond_common_la_LIBADD = \
    $(GSIGNOND_LIBS) \
    $(top_builddir)/src/common/db/libgsignond-common-db.la \
    $(NULL)

libgsignond_common_la_SOURCES = \
    gsignond-security-context.c \
    gsignond-access-control-manager.c \
    gsignond-extension-interface.c \
    gsignond-storage-manager.c \
    gsignond-identity-info-internal.h \
    gsignond-identity-info.c \
    gsignond-credentials.c \
    gsignond-config.c \
    gsignond-error.c \
    gsignond-plugin-interface.c \
    gsignond-dictionary.c \
    gsignond-session-data.c \
    gsignond-signonui-data.c \
    gsignond-plugin-loader.h \
    gsignond-plugin-loader.c \
    gsignond-utils.c \
    gsignond-pipe-stream.h \
    gsignond-pipe-stream.c \
    gsignond-disposable.h \
    gsignond-disposable.c \
    $(BUILT_SOURCES) \
    $(NULL)

dist_libgsignond_common_la_SOURCES = \
    gsignond-enum.c.template \
    gsignond-enum.h.template \
    $(NULL)

CLEANFILES = \
    gsignond-plugin-enum-types.h \
    gsignond-plugin-enum-types.c \
    $(NULL)
