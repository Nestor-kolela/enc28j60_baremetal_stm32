#!/usr/bin/env bash

BROKER="${1:-54.36.178.49}"
TOPIC="${2:-franzkafka}"
PERIOD_MS="${3:-200}"

randstr() {
    head -c "$1" /dev/urandom | base64 | tr -d '\n' | head -c "$1"
}

make_message() {
    case $((RANDOM % 6)) in
        0) echo "N:$COUNT" ;;
        1) echo "T:$COUNT,$(date +%s%3N)" ;;
        2) echo "A:$COUNT:$(randstr 64)" ;;
        3) echo "L600:$COUNT:$(randstr 600)" ;;
        4) echo "L1000:$COUNT:$(randstr 1000)" ;;
        5) echo "L1200:$COUNT:$(randstr 1200)" ;;
    esac
}

COUNT=0
echo "Starting paced publisher (${PERIOD_MS}ms)..."

while true; do
    MSG=$(make_message)

    mosquitto_pub -h "$BROKER" -t "$TOPIC" -m "$MSG" -q 0

    COUNT=$((COUNT+1))
    sleep $(awk "BEGIN {print $PERIOD_MS/1000}")
done

