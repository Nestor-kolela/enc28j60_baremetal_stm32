#!/bin/bash

while true; do
mosquitto_pub -h test.mosquitto.org -t franzkafka/ping -m test
sleep 1
done
