#!/usr/bin/env bash
set -e

mkdir -p QuantumDice/build
arduino-cli compile --jobs 0 --profile esp32s3_dev_module --build-path QuantumDice/build QuantumDice