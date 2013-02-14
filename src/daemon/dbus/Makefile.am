NULL=
INTERFACES_DIR = $(top_srcdir)/src/daemon/dbus/interfaces
SERVCIES_DIR   = $(top_srcdir)/src/daemon/dbus/services
DBUS_BUILT_SOURCES = \
    gsignond-dbus-auth-service-gen.c \
    gsignond-dbus-auth-service-gen.h \
    gsignond-dbus-auth-session-gen.c \
    gsignond-dbus-auth-session-gen.h \
    gsignond-dbus-identity-gen.c \
    gsignond-dbus-identity-gen.h \
    $(NULL)

DBUS_BUILT_DOCS = \
    gsignond-dbus-auth-service-doc-gen-com.google.code.AccountsSSO.gSingleSignOn.AuthService.xml \
    gsignond-dbus-auth-session-doc-gen-com.google.code.AccountsSSO.gSingleSignOn.AuthSession.xml \
    gsignond-dbus-identity-doc-gen-com.google.code.AccountsSSO.gSingleSignOn.Identity.xml \
    $(NULL)

DBUS_INTERFACE_PREFIX="com.google.code.AccountsSSO.gSingleSignOn."
gsignond-dbus-auth-service-gen.c gsignond-dbus-auth-service-gen.h : $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.AuthService.xml
	gdbus-codegen                                       \
       --interface-prefix $(DBUS_INTERFACE_PREFIX)      \
       --c-namespace GSignondDbus                       \
       --generate-c-code  gsignond-dbus-auth-service-gen     \
       --generate-docbook gsignond-dbus-auth-service-doc-gen \
       $<

gsignond-dbus-auth-session-gen.c gsignond-dbus-auth-session-gen.h : $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.AuthSession.xml
	gdbus-codegen                                       \
       --interface-prefix $(DBUS_INTERFACE_PREFIX)      \
       --c-namespace GSignondDbus                           \
       --generate-c-code  gsignond-dbus-auth-session-gen     \
       --generate-docbook gsignond-dbus-auth-session-doc-gen \
       $<

gsignond-dbus-identity-gen.c gsignond-dbus-identity-gen.h : $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.Identity.xml
	gdbus-codegen                                       \
       --interface-prefix $(DBUS_INTERFACE_PREFIX)      \
       --c-namespace GSignondDbus                           \
       --generate-c-code  gsignond-dbus-identity-gen         \
       --generate-docbook gsignond-dbus-identity-doc-gen     \
       $<

lib_LTLIBRARIES = libgsignond-dbus-glue.la

libgsignond_dbus_glue_la_CPPFLAGS = \
    -I$(top_srcdir) \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libgsignond_dbus_glue_la_LIBS = \
    $(GSIGNOND_LIBS) \
    $(NULL)

libgsignond_dbus_glue_la_SOURCES = $(DBUS_BUILT_SOURCES)

lib_LTLIBRARIES += libgsignond-dbus.la

libgsignond_dbus_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libgsignond_dbus_la_LIBS = \
    $(top_srcdir)/src/daemon/dbus/libgsignond_dbus_glue.so \
    $(GSIGNOND_LIBS) \
    $(NULL)

libgsignond_dbus_la_SOURCES = \
   $(DBUS_BUILT_SOURCES) \
   gsignond-dbus.h \
   gsignond-dbus-auth-service-adapter.c \
   gsignond-dbus-auth-service-adapter.h \
   gsignond-dbus-auth-session-adapter.c \
   gsignond-dbus-auth-session-adapter.h \
   gsignond-dbus-identity-adapter.c \
   gsignond-dbus-identity-adapter.h \
   $(NULL)

dbusservicedir = $(DBUS_SERVICES_DIR)
dbusservice_DATA = $(SERVCIES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.service

dbusinterfacesdir = $(DBUS_INTERFACES_DIR)
dbusinterfaces_DATA = \
    $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.AuthService.xml \
    $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.AuthSession.xml \
    $(INTERFACES_DIR)/com.google.code.AccountsSSO.gSingleSignOn.Identity.xml 

EXTRA_DIST = dbusservice

clean-local :
	rm -f *~ $(DBUS_BUILT_SOURCES) $(DBUS_BUILT_DOCS)

