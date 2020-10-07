// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "execute.hpp"
#include "value.hpp"
#include <gmock/gmock.h>
#include <test/utils/floating_point_utils.hpp>
#include <iosfwd>

MATCHER(Traps, "")  // NOLINT(readability-redundant-string-init)
{
    return arg.trapped;
}

MATCHER(Result, "empty result")
{
    return !arg.trapped && !arg.has_value;
}

MATCHER_P(Result, value, "")  // NOLINT(readability-redundant-string-init)
{
    if (arg.trapped || !arg.has_value)
        return false;

    if constexpr (std::is_floating_point_v<value_type>)
        return arg.value.template as<value_type>() == fizzy::test::FP{value};
    else  // always check 64 bit of result for all integers, including 32-bit results
        return arg.value.template as<value_type>() == value;
}

#define EXPECT_THROW_MESSAGE(stmt, ex_type, expected)                                        \
    try                                                                                      \
    {                                                                                        \
        stmt;                                                                                \
        ADD_FAILURE() << "Exception of type " #ex_type " is expected, but none was thrown."; \
    }                                                                                        \
    catch (const ex_type& exception)                                                         \
    {                                                                                        \
        EXPECT_STREQ(exception.what(), expected);                                            \
    }                                                                                        \
    catch (const std::exception& exception)                                                  \
    {                                                                                        \
        ADD_FAILURE() << "Unexpected exception type thrown: " << exception.what() << ".";    \
    }                                                                                        \
    catch (...)                                                                              \
    {                                                                                        \
        ADD_FAILURE() << "Unexpected exception type thrown.";                                \
    }                                                                                        \
    (void)0

namespace fizzy
{
std::ostream& operator<<(std::ostream& os, ExecutionResult);
}

namespace fizzy::test
{
inline uint32_t as_uint32(fizzy::Value value)
{
    return value.i32;
}
}  // namespace fizzy::test
