#!/usr/bin/sh
#ask for sudo et elevate privilege
[ "$(whoami)" != "root" ] && exec sudo -- "$0" "$@"

GIT_EXISTS=true
BAZEL_EXISTS=true
GCC_EXISTS=true
GPP_EXISTS=true

if ! [ -x "$(command -v git)" ]; then
    GIT_EXISTS=false
fi

if ! [ -x "$(command -v bazel)" ]; then
    BAZEL_EXISTS=false
fi

if ! [ -x "$(command -v gcc)" ]; then
    GCC_EXISTS=false
fi

if ! [ -x "$(command -v g++)" ]; then
    GPP_EXISTS=false
fi

if $GCC_EXISTS; then
    echo "gcc was found!"
else
    apt install gcc
fi

if $GPP_EXISTS; then
    echo "g++ was found!"
else
    apt install g++
fi

if $BAZEL_EXISTS; then
    echo "bazel was found!"
else
    apt install apt-transport-https curl gnupg
    curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor > bazel.gpg
    mv bazel.gpg /etc/apt/trusted.gpg.d/
    echo "deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8" | tee /etc/apt/sources.list.d/bazel.list
    apt update && sudo apt install bazel
    apt update && sudo apt full-upgrade
    apt install bazel
fi

if $GIT_EXISTS; then
    echo "git was found!"
else
    apt install git
fi

chmod +x build_and_run_tests.sh
chmod +x build_and_run_tests_without_clean.sh