cc_test(
    name = "ipc_sender_tests",
    srcs = [
        "ipcMessageTest.cpp",
        "ipcPipeTest.cpp",
        "ipcQueueTest.cpp",
        "ipcShmTest.cpp"
    ],
    copts = [
        "-Ilib/ipc_lib",
        "-std=c++2a",
        "-DUNIT_TESTS_BUILD"
    ],
    linkopts = ["-lrt", "-lpthread"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "//lib/ipc_lib:ipc_lib",
    ]
)

