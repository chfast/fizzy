// Fizzy: A fast WebAssembly interpreter
// Copyright 2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// The opaque data type representing a module.
struct FizzyModule;

/// The opaque data type representing an instance (instantiated module).
struct FizzyInstance;

/// The data type representing numeric values.
///
/// i64 member is used to represent values of both i32 and i64 type.
union FizzyValue
{
    uint64_t i64;
    float f32;
    double f64;
};

/// Result of execution of a function.
typedef struct FizzyExecutionResult
{
    /// Whether execution ended with a trap.
    bool trapped;
    /// Whether function returned a value. Valid only if trapped == false
    bool has_value;
    /// Value returned from a function. Valid only if has_value == true
    union FizzyValue value;
} FizzyExecutionResult;


/// Pointer to external function.
///
/// @param context      Opaque pointer to execution context.
/// @param instance     Pointer to module instance.
/// @param args         Pointer to the argument array. Can be NULL if function has 0 inputs.
/// @param args_size    Size of the  argument array.
/// @param depth        Call stack depth.
typedef struct FizzyExecutionResult (*FizzyExternalFn)(void* context,
    struct FizzyInstance* instance, const union FizzyValue* args, size_t args_size, int depth);

/// Value type.
enum FizzyValueType
{
    FizzyValueTypeI32 = 0x7f,
    FizzyValueTypeI64 = 0x7e,
    FizzyValueTypeF32 = 0x7d,
    FizzyValueTypeF64 = 0x7c,
};

/// Function type.
typedef struct FizzyFunctionType
{
    /// Pointer to input types array.
    const enum FizzyValueType* inputs;
    /// Input types array size.
    size_t inputs_size;
    /// Pointer to output types array.
    const enum FizzyValueType* outputs;
    /// Output types array size.
    size_t outputs_size;
} FizzyFunctionType;

/// External function.
typedef struct FizzyExternalFunction
{
    /// Function type.
    FizzyFunctionType type;
    /// Pointer to function.
    FizzyExternalFn function;
    /// Opaque pointer to execution context, that will be passed to function.
    void* context;
} FizzyExternalFunction;

/// Validate binary module.
bool fizzy_validate(const uint8_t* wasm_binary, size_t wasm_binary_size);

/// Parse binary module.
const struct FizzyModule* fizzy_parse(const uint8_t* wasm_binary, size_t wasm_binary_size);

/// Free resources associated with the module.
///
/// Should be called unless @p module was passed to fizzy_instantiate.
void fizzy_free_module(const struct FizzyModule* module);

/// Get type of the function defined in the module.
///
/// @param module   Pointer to module.
/// @param func_idx Function index. Can be index of an imported function.
struct FizzyFunctionType fizzy_get_function_type(
    const struct FizzyModule* module, uint32_t func_idx);

bool fizzy_find_exported_function(
    const struct FizzyModule* module, const char* name, uint32_t* out_func_idx);

/// Instantiate a module.
/// Takes ownership of module, i.e. @p module is invalidated after this call.
///
/// @param module                   Pointer to module.
/// @param imported_functions       Pointer to the imported function array.
/// @param imported_functions_size  Size of the imported function array.
struct FizzyInstance* fizzy_instantiate(const struct FizzyModule* module,
    const struct FizzyExternalFunction* imported_functions, size_t imported_functions_size);

/// Free resources associated with the instance.
void fizzy_free_instance(struct FizzyInstance* instance);

const struct FizzyModule* fizzy_get_instance_module(struct FizzyInstance* instance);

uint8_t* fizzy_get_instance_memory_data(struct FizzyInstance* instance);

size_t fizzy_get_instance_memory_size(struct FizzyInstance* instance);

/// Execute module function.
///
/// @param instance     Pointer to module instance.
/// @param args         Pointer to the argument array. Can be NULL if function has 0 inputs.
/// @param depth        Call stack depth.
FizzyExecutionResult fizzy_execute(
    struct FizzyInstance* instance, uint32_t func_idx, const union FizzyValue* args, int depth);

#ifdef __cplusplus
}
#endif
