#!/bin/sh

case "$1" in
    start)
        echo "Starting aesdsocket"
        #start-stop-daemon --start --name aesdsocket --startas /usr/bin/aesdsocket -- -d
        start-stop-daemon --start --exec /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket"
        start-stop-daemon --stop --name aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac

exit 0