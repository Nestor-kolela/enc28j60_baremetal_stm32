#!/bin/bash

BROKER=54.36.178.49
TOPIC=franzkafka
COUNT=0

echo "Starting infinite soak test..."

while true; do
    COUNT=$((COUNT+1))
    mosquitto_pub -h $BROKER -t $TOPIC -m "$COUNT"
    sleep 0.2
done

