#!/usr/bin/sh
bazel build //src/ipc_receiver:ipc_receiver 
bazel build //src/ipc_sender:ipc_sender

bazel test //test:ipc_tests --test_output=all
rm -rf output

mkdir output

cp bazel-bin/src/ipc_sender/ipc_sender output
cp bazel-bin/src/ipc_receiver/ipc_receiver output
cp bazel-testlogs/gtest/Gtest_ipc/test.log output