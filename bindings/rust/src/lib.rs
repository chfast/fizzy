// Fizzy: A fast WebAssembly interpreter
// Copyright 2019-2020 The Fizzy Authors.
// SPDX-License-Identifier: Apache-2.0

//! This is a Rust interface to [Fizzy](https://github.com/wasmx/fizzy), a WebAssembly virtual machine.
//!
//! Currently it is only possible to validate inputs, but soon execution will be supported too.

mod sys;

use std::ptr::NonNull;

/// Parse and validate the input according to WebAssembly 1.0 rules. Returns true if the supplied input is valid.
pub fn validate<T: AsRef<[u8]>>(input: T) -> bool {
    unsafe { sys::fizzy_validate(input.as_ref().as_ptr(), input.as_ref().len()) }
}

pub struct Module {
    ptr: NonNull<sys::FizzyModule>,
}

impl Drop for Module {
    fn drop(&mut self) {
        unsafe { sys::fizzy_free_module(self.ptr.as_ptr()) }
    }
}

pub fn parse<T: AsRef<[u8]>>(input: &T) -> Result<Module, ()> {
    let ptr = unsafe { sys::fizzy_parse(input.as_ref().as_ptr(), input.as_ref().len()) };
    if ptr.is_null() {
        return Err(());
    }
    Ok(Module {
        ptr: unsafe { NonNull::new_unchecked(ptr) },
    })
}

pub struct Instance {
    ptr: NonNull<sys::FizzyInstance>,
}

impl Drop for Instance {
    fn drop(&mut self) {
        unsafe { sys::fizzy_free_instance(self.ptr.as_ptr()) }
    }
}

impl Module {
    // TODO: support imported functions{
    pub fn instantiate(self) -> Result<Instance, ()> {
        let ptr = unsafe { sys::fizzy_instantiate(self.ptr.as_ptr(), std::ptr::null_mut(), 0) };
        // Forget Module (and avoid calling drop) because it has been consumed by instantiate (even if it failed).
        core::mem::forget(self);
        if ptr.is_null() {
            return Err(());
        }
        Ok(Instance {
            ptr: unsafe { NonNull::new_unchecked(ptr) },
        })
    }
}

// TODO: add a proper wrapper (enum type for Value, and Result<Value, Trap> for ExecutionResult)

pub type Value = sys::FizzyValue;

// NOTE: the union does not have i32
impl Value {
    pub fn as_i32(&self) -> i32 {
        unsafe { self.i64 as i32 }
    }
    pub fn as_u32(&self) -> u32 {
        unsafe { self.i64 as u32 }
    }
    pub fn as_i64(&self) -> i64 {
        unsafe { self.i64 as i64 }
    }
    pub fn as_u64(&self) -> u64 {
        unsafe { self.i64 }
    }
    pub fn as_f32(&self) -> f32 {
        unsafe { self.f32 }
    }
    pub fn as_f64(&self) -> f64 {
        unsafe { self.f64 }
    }
}

impl From<i32> for Value {
    fn from(v: i32) -> Self {
        Value { i64: v as u64 }
    }
}

impl From<u32> for Value {
    fn from(v: u32) -> Self {
        Value { i64: v as u64 }
    }
}

impl From<i64> for Value {
    fn from(v: i64) -> Self {
        Value { i64: v as u64 }
    }
}

impl From<u64> for Value {
    fn from(v: u64) -> Self {
        Value { i64: v }
    }
}
impl From<f32> for Value {
    fn from(v: f32) -> Self {
        Value { f32: v }
    }
}

impl From<f64> for Value {
    fn from(v: f64) -> Self {
        Value { f64: v }
    }
}

pub struct ExecutionResult(sys::FizzyExecutionResult);

impl ExecutionResult {
    pub fn trapped(&self) -> bool {
        self.0.trapped
    }
    pub fn value(&self) -> Option<Value> {
        if self.0.has_value {
            return Some(self.0.value);
        }
        None
    }
}

impl From<ExecutionResult> for sys::FizzyExecutionResult {
    fn from(v: ExecutionResult) -> Self {
        v.0
    }
}

impl Instance {
    pub unsafe fn unsafe_execute(&mut self, func_idx: u32, args: &[Value]) -> ExecutionResult {
        ExecutionResult {
            0: sys::fizzy_execute(self.ptr.as_ptr(), func_idx, args.as_ptr(), 0),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validate_wasm() {
        // Empty
        assert_eq!(validate(&[]), false);
        // Too short
        assert_eq!(validate(&[0x00]), false);
        // Valid
        assert_eq!(
            validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]),
            true
        );
        // Invalid version
        assert_eq!(
            validate(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x01]),
            false
        );
    }

    #[test]
    fn parse_wasm() {
        assert!(parse(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]).is_ok());
        assert!(parse(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x01]).is_err());
    }

    #[test]
    fn instantiate_wasm() {
        let module = parse(&[0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00]);
        assert!(module.is_ok());
        let instance = module.unwrap().instantiate();
        assert!(instance.is_ok());
    }

    #[test]
    fn execute_wasm() {
        let input = hex::decode("0061736d01000000010e036000006000017f60027f7f017f030504000102000a150402000b0400412a0b0700200020016e0b0300000b").unwrap();
        let module = parse(&input);
        assert!(module.is_ok());
        let instance = module.unwrap().instantiate();
        assert!(instance.is_ok());
        let mut instance = instance.unwrap();

        let result = unsafe { instance.unsafe_execute(0, &[]) };
        assert!(!result.trapped());
        assert!(!result.value().is_some());

        let result = unsafe { instance.unsafe_execute(1, &[]) };
        assert!(!result.trapped());
        assert!(result.value().is_some());
        assert_eq!(result.value().unwrap().as_i32(), 42);

        // Explicit type specification
        let result =
            unsafe { instance.unsafe_execute(2, &[(42 as i32).into(), (2 as i32).into()]) };
        assert!(!result.trapped());
        assert!(result.value().is_some());
        assert_eq!(result.value().unwrap().as_i32(), 21);

        // Implicit i64 types (even though the code expects i32)
        let result = unsafe { instance.unsafe_execute(2, &[42.into(), 2.into()]) };
        assert!(!result.trapped());
        assert!(result.value().is_some());
        assert_eq!(result.value().unwrap().as_i32(), 21);

        let result = unsafe { instance.unsafe_execute(3, &[]) };
        assert!(result.trapped());
        assert!(!result.value().is_some());
    }
}
