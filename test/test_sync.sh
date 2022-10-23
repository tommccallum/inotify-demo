#!/usr/bin/env bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
PROJECT_DIR="${SCRIPT_DIR}/.."
TEST_DIR="${PROJECT_DIR}/test"
TEST_FILE_DIR="${PROJECT_DIR}/test_demo"


[[ -e "${TEST_FILE_DIR}/test_target" ]] && rm -rf "${TEST_FILE_DIR}/test_target"
[[ ! -e "${TEST_FILE_DIR}/test_target" ]] && mkdir "${TEST_FILE_DIR}/test_target" 


$PROJECT_DIR/watcher --source "${TEST_FILE_DIR}/test_source" "${TEST_FILE_DIR}/test_target" --sync --test-mode 1
find "${TEST_FILE_DIR}/test_source" > ${TEST_DIR}/a.txt
find "${TEST_FILE_DIR}/test_target" > ${TEST_DIR}/b.txt
sed -i "s/test_target/test_source/g" ${TEST_DIR}/b.txt
diff ${TEST_DIR}/a.txt ${TEST_DIR}/b.txt
if [ $? -ne 0 ]; then
    echo "Failed"
else
    echo "Success"
fi
rm -f ${TEST_DIR}/a.txt
rm -f ${TEST_DIR}/b.txt


