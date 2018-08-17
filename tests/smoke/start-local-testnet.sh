#!/bin/bash

set -vex

echo "starting local regression test nodes"

cd /home/dev/nexustest

cd testnet1

./run-test-nexus.sh &

cd ../testnet2

./run-test-nexus.sh &

cd $HOME/code/Nexus/qa/smoke

./startminerfortest.sh &

exit 0
