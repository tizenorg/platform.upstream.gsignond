SRC_HOME="."
with_duma=0
with_gdb=0

if test $# -ge 1 ; then
    if test "$1" == "--with-duma" ; then
        with_duma=1
        with_gdb=0
    else
        with_duma=0
        if test "$1" == "--with-gdb" ; then
            with_gdb=1
        fi
    fi
fi

killall gsignond

export SSO_STORAGE_PATH="/tmp/gsignond"
if [ -f "$SRC_HOME/test/daemon/.libs/lt-daemontest" ] ; then
export SSO_KEYCHAIN_SYSCTX="$SRC_HOME/test/daemon/.libs/lt-daemontest"
else
export SSO_KEYCHAIN_SYSCTX="$SRC_HOME/test/daemon/.libs/daemontest"
fi
export SSO_BIN_DIR=$SRC_HOME/src/daemon/.libs
export SSO_PLUGINS_DIR=$SRC_HOME/src/plugins/.libs 
export LD_LIBRARY_PATH="$SRC_HOME/src/daemon/plugins/plugind/.libs:$SRC_HOME/src/daemon/plugins/.libs"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SRC_HOME/src/common/.libs:$SRC_HOME/src/common/db/.libs:$SRC_HOME/src/daemon/.libs:$SRC_HOME/src/daemon/db/.libs:$SRC_HOME/src/daemon/dbus/.libs"
export G_MESSAGES_DEBUG="all"

# Clean db
#rm -rf /tmp/gsignond

echo "--------------------------"
echo "with_duma:  $with_duma"
echo "with_gdb:  $with_gdb"
echo "--------------------------"
if test $with_duma -eq 1 ; then
    export G_SLICE="always-malloc"
    export DUMA_PROTECT_FREE=1
    export DUMA_PROTECT_BELOW=1

    LD_PRELOAD="libduma.so" $SRC_HOME/src/daemon/.libs/gsignond  

    if test $with_gdb -eq 1 ; then
        sudo gdb --pid=`pidof gsignond`
    fi
elif test $with_gdb -eq 1 ; then
    gdb $SRC_HOME/src/daemon/.libs/gsignond
else
    $SRC_HOME/src/daemon/.libs/gsignond
fi

