#! /bin/sh

#taken directly from Linux System Initialization, 3:35
case "$1" in
    start)
        echo "Starting aarch64-aesdsocket"
        start-stop-daemon -S -n aarch64-aesdsocket -a /usr/bin/aarch64-aesdsocket -- -d
        ;;
    stop)
        echo "Stoping aarch64-aesdsocket"
        start-stop-daemon -K -n aarch64-aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0