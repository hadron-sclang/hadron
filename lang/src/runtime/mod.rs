//! Tools supporting the execution of SuperCollider code.
//!
//! The SuperCollider language requires a runtime environment, including an object dynamic dispatch
//! and reflection, garbage collection, soft real-time clocks and signaling, and others. This
//! module implements those requirements.
//!

pub mod slot;
