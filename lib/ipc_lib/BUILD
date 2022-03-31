cc_library(
    name = "ipc_lib",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
    deps = ["@com_github_google_glog//:glog"],
    copts = ["-std=c++2a"],
    linkopts = ["-lrt", "-lpthread"]
    
)