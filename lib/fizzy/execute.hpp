// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cxx20/span.hpp"
#include "exceptions.hpp"
#include "instantiate.hpp"
#include "limits.hpp"
#include "module.hpp"
#include "types.hpp"
#include "value.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace fizzy
{
/// The result of an execution.
struct ExecutionResult
{
    /// This is true if the execution has trapped. No other fields are valid.
    const bool trapped = false;
    /// This is true if the `value()` returns valid data.
    const bool has_value = false;
    /// This single result of execution.
    const Value value{};

    /// Constructs result with a value.
    constexpr ExecutionResult(Value _value) noexcept : has_value{true}, value{_value} {}

    /// Constructs result in "void" or "trap" state depending on the success flag.
    /// Prefer using Void and Trap constants instead.
    constexpr explicit ExecutionResult(bool success) noexcept : trapped{!success} {}
};

constexpr ExecutionResult Void{true};
constexpr ExecutionResult Trap{false};

/// Execute a function on an instance.
///
/// @param instance The instance, see `instance.hpp`.
/// @param func_idx The function index. MUST be a valid index, otherwise undefined behaviour
/// (including crash) happens.
/// @param args     The pointer to the arguments. The number of items must match the expected number
/// of input parameters of the function, otherwide underfiend behaviour (including crash) happens.
/// @param depth    The call depth (indexing starts at 0). Can be left at the default setting.
ExecutionResult execute(Instance& instance, FuncIdx func_idx, const Value* args, int depth = 0);

/// Execute a function on an instance.
///
/// @param instance The instance, see `instance.hpp`.
/// @param func_idx The function index. MUST be a valid index, otherwise undefined behaviour
/// (including crash) happens.
/// @param args     The arguments. The number of items must match the expected number
/// of input parameters of the function, otherwide underfiend behaviour (including crash) happens.
inline ExecutionResult execute(
    Instance& instance, FuncIdx func_idx, std::initializer_list<Value> args)
{
    assert(args.size() == instance.module->get_function_type(func_idx).inputs.size());
    return execute(instance, func_idx, args.begin());
}
}  // namespace fizzy
