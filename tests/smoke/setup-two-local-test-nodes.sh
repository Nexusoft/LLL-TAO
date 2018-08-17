#!/bin/bash

# Quick script to setup nexus for step through debugging.

NEXUS_TEST_CLEAR=1 NEXUS_TEST_SETUP_ONLY=1 NEXUS_TEST_NO_DAEMON=1 ./run-start-nexus-test.sh

NEXUS_TEST_NODE_NUM=2 NEXUS_TEST_CLEAR=1 NEXUS_TEST_SETUP_ONLY=1 NEXUS_TEST_NO_DAEMON=1 ./run-start-nexus-test.sh
