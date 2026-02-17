#!/usr/bin/env bash

BROKER="${1:-54.36.178.49}"
TOPIC="${2:-franzkafka}"
SIZE="${3:-1024}"   # bytes: try 1024 or 1200

# generate exactly SIZE printable bytes
PAYLOAD=$(head -c "$SIZE" /dev/urandom | base64 | tr -d '\n' | head -c "$SIZE")

echo "Sending MQTT message"
echo "Broker : $BROKER"
echo "Topic  : $TOPIC"
echo "Length : $SIZE bytes"

mosquitto_pub -h "$BROKER" -t "$TOPIC" -m "$PAYLOAD" -q 0
