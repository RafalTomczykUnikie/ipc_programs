cc_library(
    name = "ipc_lib",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
    deps = [
        "@com_github_google_glog//:glog",
        "@com_google_googletest//:gtest_main"
    ],
    copts = ["-std=c++2a", "-DUNIT_TESTS_BUILD"],
    linkopts = ["-lrt", "-lpthread"]
    
)