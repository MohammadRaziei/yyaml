#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Cloning yaml-test-suite into $SCRIPT_DIR ..."

git clone https://github.com/yaml/yaml-test-suite.git "$SCRIPT_DIR/yaml-test-suite"

echo "Done!"