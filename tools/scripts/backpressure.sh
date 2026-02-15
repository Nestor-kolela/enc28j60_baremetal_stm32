#!/bin/bash
while true; do
mosquitto_pub -h test.mosquitto.org -t franzkafka/spam -m x
done
