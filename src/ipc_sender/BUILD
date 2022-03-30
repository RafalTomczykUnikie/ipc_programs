cc_binary(
    name = "ipc_sender",
    srcs = ["main.cpp"],
    deps = [
        "//lib/ipc_lib:ipc_lib",
        "@com_github_google_glog//:glog"
    ],
    copts = [
        "-Ilib/ipc_lib",
        "-std=c++2a",
    ],
    linkopts = ["-lrt"]
)