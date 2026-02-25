#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# -----------------------------------------
# Function
# -----------------------------------------
run_if_not_exists() {
    local folder_name="$1"
    local command_template="$2"

    local full_path="$SCRIPT_DIR/$folder_name"

    if [ -d "$full_path" ]; then
        echo "📁 '$folder_name' already exists → Skipping"
    else
        echo "🚀 Executing for '$folder_name'..."

        local command="${command_template//\{\}/$full_path}"

        echo "👉 $command"
        eval "$command"

        echo "✅ Done"
    fi

    echo "--------------------------------------"
}

# -----------------------------------------
# Usage
# -----------------------------------------

run_if_not_exists "yaml-test-suite" \
    "git clone --depth=1 https://github.com/yaml/yaml-test-suite.git {}"

run_if_not_exists "apis-guru-openapi-directory" \
    "git clone --depth=1 https://github.com/APIs-guru/openapi-directory.git {}"

echo "🎉 All tasks finished!"