SUBDIRS = db dbus
NULL=

bin_PROGRAMS = gsignond

gsignond_SOURCES = \
    main.c \
    gsignond-auth-session.c \
    gsignond-auth-session.h \
    gsignond-auth-service-iface.c \
    gsignond-auth-service-iface.h \
    gsignond-auth-session-iface.c \
    gsignond-auth-session-iface.h \
    gsignond-daemon.c \
    gsignond-daemon.h \
    gsignond-identity-iface.c \
    gsignond-identity-iface.h \
    gsignond-identity.c \
    gsignond-identity.h \
    $(NULL)

gsignond_CFLAGS = \
    -DG_LOG_DOMAIN=\"gsignon-daemon\" \
    -DGSIGNOND_EXTENSIONS_DIR="\"$(libdir)/signon/extensions\"" \
    -I$(top_srcdir) \
    -I$(top_builddir)/include/ \
    -I$(top_builddir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

gsignond_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/db/libgsignond-db.la \
    $(top_builddir)/src/daemon/dbus/libgsignond-dbus.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

