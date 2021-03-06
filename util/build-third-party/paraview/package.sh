#!/bin/bash
# -----------------------------------------------------------------------------
#
# Copyright (C) 2021 CERN & Newcastle University for the benefit of the
# BioDynaMo collaboration. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
# See the LICENSE file distributed with this work for details.
# See the NOTICE file distributed with this work for additional information
# regarding copyright ownership.
#
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "${SCRIPT_DIR}/env.sh"

set -e -x

cd "$SCRIPT_DIR/build/install"

## tar the install directory
RESULT_FILE=paraview-$PV_VERSION-$BDM_OS-$PV_FLAVOR.tar.gz 
tar -zcf $RESULT_FILE *
shasum -a256 ${RESULT_FILE} > ${RESULT_FILE}.sha256

# After untarring the directory tree should like like this:
# paraview
#   |-- bin
#   |-- include
#   |-- lib
#   |-- share

# Step 5: cp to destination directory
cp $RESULT_FILE $BDM_PROJECT_DIR/build
cp ${RESULT_FILE}.sha256 $BDM_PROJECT_DIR/build

