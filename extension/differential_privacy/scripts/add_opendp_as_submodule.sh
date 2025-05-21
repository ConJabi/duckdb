#!/bin/bash

# Define OpenDP repository URL and target directory
OPENDP_REPO="https://github.com/opendp/opendp.git"
SUBMODULE_DIR="opendp"

# Ensure script stops on error
set -e

echo "Adding OpenDP as a submodule..."
git submodule add -f --depth=1 $OPENDP_REPO $SUBMODULE_DIR

echo "Initializing submodule..."
git submodule update --init --recursive

# Navigate to the submodule directory
cd $SUBMODULE_DIR || exit

echo "Enabling sparse checkout (only fetching /rust)..."
git sparse-checkout init --cone
git sparse-checkout set rust

# Fetch and check out the main branch of OpenDP
echo "Fetching OpenDP repository..."
git fetch origin
git checkout -b opendp-updates origin/main

echo "Creating a branch for custom changes..."
git checkout -b my-custom-changes

# Navigate back to the main project
cd ..

echo "Committing the submodule reference..."
git add $SUBMODULE_DIR
git commit -m "Added OpenDP submodule with only /rust and initialized tracking branches"

echo "Setup complete! Only /rust is included from OpenDP."
