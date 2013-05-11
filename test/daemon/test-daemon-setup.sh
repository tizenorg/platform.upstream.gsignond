#Environment variables for the test application
#export GSIGNOND_CONFIG=../../test/daemon/test-daemon.conf
export G_MESSAGES_DEBUG=all
export SSO_IDENTITY_TIMEOUT=5
export SSO_STORAGE_PATH=/tmp/gsignond
export SSO_SECRET_PATH=/tmp/gsignond

rm -rf "$SSO_STORAGE_PATH"

TEST_APP=./daemontest

# If dbus-test-runner exists, use it to run the tests in a separate D-Bus
# session
if command -v dbus-test-runner > /dev/null ; then
    echo "Using dbus-test-runner"
    dbus-test-runner -m 180 -t signond \
        -t "$TEST_APP" -f com.google.code.AccountsSSO.gSingleSignOn
else
    echo "Using existing D-Bus session"
    killall gsignond || true
    trap "killall gsignond" EXIT
    ../../src/daemon/gsignond &
    sleep 2

    $TEST_APP
fi

