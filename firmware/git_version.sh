#!/bin/bash

# set the GIT hash first.
GITHASH=$(git rev-parse HEAD)
echo "-DVERSION_GIT_HASH=${GITHASH}"

# then report on the cleanlyness.

#http://stackoverflow.com/a/5139672
git diff --exit-code > /dev/null
RESULT=$?
echo "-DVERSION_WORKING_DIRECTORY_CHANGES_UNSTAGED=${RESULT}"

git diff --cached --exit-code > /dev/null
RESULT=$?
echo "-DVERSION_WORKING_DIRECTORY_CHANGES_STAGED=${RESULT}"
