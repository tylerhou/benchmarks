load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",
    targets = {
        "//...": "",
    },
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
)

cc_test(
    name = "main_test",
    srcs = ["main_test.cc"],
    deps = ["@googletest//:gtest_main"],
)

cc_test(
    name = "main-benchmark_test",
    srcs = ["main-benchmark_test.cc"],
    deps = ["@google_benchmark//:benchmark_main"],
)

cc_test(
    name = "vowels-benchmark_test",
    srcs = ["vowels-benchmark_test.cc"],
    deps = ["@google_benchmark//:benchmark_main"],
)
