# Fizzy: A fast WebAssembly interpreter
# Copyright 2019-2020 The Fizzy Authors.
# SPDX-License-Identifier: Apache-2.0

add_executable(fizzy-bench-internal)

target_sources(fizzy-bench-internal PRIVATE
    bench_internal.cpp
    experimental.cpp
    experimental.hpp
    parser_benchmarks.cpp
)

target_link_libraries(fizzy-bench-internal PRIVATE fizzy::fizzy-internal fizzy::test-utils benchmark::benchmark_main)
