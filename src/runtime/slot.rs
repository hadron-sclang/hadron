//! Support for the SuperCollider fundamental runtime polymorphic data structure.
//!
//! SuperCollider is a *dynamic* programming language, meaning that the types of variables don't
//! have to be specified at compile time. Hadron packs all of the SuperCollider fundamental types
//! into a type-tagged data structure called a [Slot]. The [Slot] name comes from the original
//! implementation of the SuperCollider interpreter, preserved here for clarity.
//!

/// A type-tagged data structure used to represent *all* data in the SuperCollider runtime.
///
/// The size of the [Slot] is impactful, as it sets the lower bound of the size of any
/// SuperCollider data structure. This implementation is optimized to fit a [Slot] into 8 bytes.
/// It follows from the work of Nikita Popov, discussed
/// [here](https://www.npopov.com/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html).
///
/// In short, Nikita observes that IEEE 754 double-precision floating point numbers and 64-bit
/// addresses on all modern 64-bit processors both allow for a few bits of storage near the most
/// significant bits of the word. We abuse this observation to store type tags in that space,
/// allowing us to fit all of the types enumerated in [SlotType] within the same 8-byte
/// structure, the [Slot].
#[derive(PartialEq)]
pub struct Slot {
    bits: u64,
}

// Ensure that Slot remains 8 bytes in size.
assert_eq_size!(Slot, u64);

/// Represents all possible types stored in a [Slot] with their associated data payload.
///
/// See the [Slot] methods [Slot::extract()] and [Slot::insert()] for ergonomic methods unpacking
/// and packing the type and associated data from a [Slot] into a [SlotType].
#[derive(PartialEq)]
pub enum SlotType {
    Boolean(bool),
    Character(char),
    Float(f64),
    Integer(i32),
    Nil,
}

impl Slot {
    const MAX_DOUBLE: u64 = 0xfff8000000000000;

    const BOOLEAN_TAG: u64 = Slot::MAX_DOUBLE;
    const CHARACTER_TAG: u64 = 0xfff9000000000000;
    const INTEGER_TAG: u64 = 0xfffa000000000000;
    const NIL_TAG: u64 = 0xfffb000000000000;
    //  const OBJECT_TAG: u64 = 0xfffc000000000000;
    //  const SYMBOL_TAG: u64 = 0xfffd000000000000;
    const TAG_MASK: u64 = 0xffff000000000000;

    /// Extracts a type and associated data from the [Slot] as a [SlotType].
    ///
    /// This method and its inverse method [Slot::insert()] are the primary Rust-facing functions
    /// for reading and writing Slot values, including compile-time type information.
    ///
    /// # Panics
    ///
    /// [Slot::extract()] will panic if it encounters a type tag in the [Slot] that it doesn't
    /// recognize.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// // Add an integer to a Slot.
    /// fn add_integer(operand: i32, slot: Slot) -> Slot {
    ///     match slot.extract() {
    ///         SlotType::Integer(i) => Slot::from_integer(i + operand),
    ///         SlotType::Float(f) => Slot::from_integer((f as i32) + operand),
    ///         _ => panic!("tried to add non-numeric type and integer")
    ///     }
    /// }
    /// ```
    pub fn extract(&self) -> SlotType {
        if self.is_float() {
            return SlotType::Float(self.as_float_unchecked());
        }
        match self.bits & Slot::TAG_MASK {
            Slot::BOOLEAN_TAG => SlotType::Boolean(self.as_boolean_unchecked()),
            Slot::CHARACTER_TAG => SlotType::Character(self.as_character_unchecked()),
            Slot::INTEGER_TAG => SlotType::Integer(self.as_integer_unchecked()),
            Slot::NIL_TAG => SlotType::Nil,
            _ => panic!("unknown type tag 0x{:016x} in Slot", self.bits),
        }
    }

    /// Inserts the type and associated data from a [SlotType] into a [Slot].
    ///
    /// This method and its inverse method [Slot::extract()] are the primary Rust-facing functions
    /// for reading and writing Slot values, including compile-time type information.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// // Make a type-appropriate zero or nil type from the supplied Slot.
    /// fn nullify(slot: Slot) -> Slot {
    ///     let slot_type = match slot.extract() {
    ///         SlotType::Boolean(_) => SlotType::Boolean(false),
    ///         SlotType::Character(_) => SlotType::Character('\0'),
    ///         SlotType::Float(_) => SlotType::Float(0.0),
    ///         SlotType::Integer(_) => SlotType::Integer(0),
    ///         SlotType::Nil => SlotType::Nil
    ///     };
    ///     Slot::insert(slot_type)
    /// }
    /// ```
    pub fn insert(val: SlotType) -> Slot {
        match val {
            SlotType::Float(f) => Slot::from_float(f),
            SlotType::Integer(i) => Slot::from_integer(i),
            SlotType::Boolean(b) => Slot::from_boolean(b),
            SlotType::Character(c) => Slot::from_character(c),
            SlotType::Nil => Slot::nil(),
        }
    }

    /// Make a [Slot] from a supplied bool value.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let slot_true = Slot::from_boolean(true);
    /// let slot_false = Slot::from_boolean(false);
    /// assert!(slot_true.as_boolean().unwrap());
    /// assert_eq!(slot_false.extract(), SlotType::Boolean(false));
    /// ```
    pub fn from_boolean(boolean: bool) -> Slot {
        match boolean {
            true => Slot {
                bits: Slot::BOOLEAN_TAG | 1,
            },
            false => Slot {
                bits: Slot::BOOLEAN_TAG,
            },
        }
    }

    /// Returns true if the [Slot] contains a boolean value.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let slot_bool = Slot::from_boolean(false);
    /// let slot_float = Slot::from_float(0.0);
    /// assert!(slot_bool.is_boolean());
    /// assert!(!slot_float.is_boolean());
    /// ```
    pub fn is_boolean(&self) -> bool {
        self.bits & Slot::TAG_MASK == Slot::BOOLEAN_TAG
    }

    /// Returns `Some(bool)` if the [Slot] contains a boolean value, `None` otherwise.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let slot = Slot::from_boolean(true);
    /// if (slot.as_boolean().unwrap()) {
    ///     println!("true!");
    /// }
    /// ```
    pub fn as_boolean(&self) -> Option<bool> {
        if self.is_boolean() {
            Some(self.as_boolean_unchecked())
        } else {
            None
        }
    }
    fn as_boolean_unchecked(&self) -> bool {
        self.bits == (Slot::BOOLEAN_TAG | 1)
    }

    /// Make a [Slot] from a supplied 32-bit character value.
    ///
    /// The legacy SuperCollider interpreter supports only 8-bit character values. However, we
    /// can store 32-bit values, so this type accepts wide UTF-8 values. The Lexer can be
    /// configured to accept wide character literals or not.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let fire = Slot::from_character('🔥');
    /// assert_eq!(fire.extract(), SlotType::Character('🔥'));
    /// ```
    pub fn from_character(character: char) -> Slot {
        let short = character as u32;
        let long = short as u64;
        Slot {
            bits: (long | Slot::CHARACTER_TAG),
        }
    }

    /// Returns true if the [Slot] holds a character.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let character = Slot::from_character('\0');
    /// let integer = Slot::from_integer(0);
    /// assert!(character.is_character());
    /// assert!(!integer.is_character());
    /// ```
    pub fn is_character(&self) -> bool {
        self.bits & Slot::TAG_MASK == Slot::CHARACTER_TAG
    }

    /// If the [Slot] holds a character, return it.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// fn to_uppercase(slot: Slot) -> Option<char> {
    ///     match slot.as_character() {
    ///         Some(c) => c.to_uppercase().next(),
    ///         None => None
    ///     }
    /// }
    /// assert_eq!(to_uppercase(Slot::from_character('a')),
    ///            Slot::from_character('A').as_character());
    /// ```
    pub fn as_character(&self) -> Option<char> {
        if self.is_character() {
            Some(self.as_character_unchecked())
        } else {
            None
        }
    }
    fn as_character_unchecked(&self) -> char {
        char::from_u32((self.bits & !Slot::TAG_MASK) as u32)
            .expect("char stored in slot should be a valid character")
    }

    /// Make a [Slot] from a double-precision floating point number.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let a = Slot::from_float(-1.23);
    /// assert_eq!(a.extract(), SlotType::Float(-1.23));
    /// ```
    pub fn from_float(float: f64) -> Slot {
        let s = unsafe { std::mem::transmute::<f64, u64>(float) };
        Slot { bits: s }
    }

    /// Returns true if this [Slot] holds a float.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let a = Slot::from_float(1e7);
    /// let b = Slot::nil();
    /// assert!(a.is_float());
    /// assert!(!b.is_float());
    /// ```
    pub fn is_float(&self) -> bool {
        self.bits < Slot::MAX_DOUBLE
    }

    /// If the [Slot] holds a float, return it.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// fn square(slot: Slot) -> Option<f64> {
    ///     match slot.as_float() {
    ///         None => None,
    ///         Some(float) => Some(float * float)
    ///     }
    /// }
    /// assert_eq!(square(Slot::from_float(-2.0)), Some(4.0));
    /// assert_eq!(square(Slot::nil()), None);
    /// ```
    pub fn as_float(&self) -> Option<f64> {
        if self.is_float() {
            Some(self.as_float_unchecked())
        } else {
            None
        }
    }
    fn as_float_unchecked(&self) -> f64 {
        unsafe { std::mem::transmute::<u64, f64>(self.bits) }
    }

    /// Make a [Slot] from an integer.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let k = Slot::from_integer(23);
    /// assert_eq!(k.extract(), SlotType::Integer(23));
    /// ```
    pub fn from_integer(int: i32) -> Slot {
        let long = int as u32 as u64;
        Slot {
            bits: long | Slot::INTEGER_TAG,
        }
    }

    /// Returns true if this [Slot] holds an integer.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let i = Slot::from_integer(0);
    /// let f = Slot::from_float(0.0);
    /// assert!(i.is_integer());
    /// assert!(!f.is_integer());
    /// assert_ne!(i, f);
    /// ```
    pub fn is_integer(&self) -> bool {
        self.bits & Slot::TAG_MASK == Slot::INTEGER_TAG
    }

    /// If the [Slot] holds an integer, return it.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// fn halve(slot: Slot) -> Option<i32> {
    ///     match slot.as_integer() {
    ///         None => None,
    ///         Some(i) => Some(i >> 1)
    ///     }
    /// }
    /// assert_eq!(halve(Slot::from_integer(2)), Some(1));
    /// assert_eq!(halve(Slot::from_integer(-2)), Some(-1));
    /// assert_eq!(halve(Slot::from_float(2.0)), None);
    /// ```
    pub fn as_integer(&self) -> Option<i32> {
        if self.is_integer() {
            Some(self.as_integer_unchecked())
        } else {
            None
        }
    }
    fn as_integer_unchecked(&self) -> i32 {
        (self.bits & !Slot::TAG_MASK) as i32
    }

    /// Create a [Slot] holding the SuperCollider nil type.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let n = Slot::nil();
    /// assert!(n.is_nil());
    /// ```
    pub fn nil() -> Slot {
        Slot {
            bits: Slot::NIL_TAG,
        }
    }

    /// Returns true if this [Slot] holds the SuperCollider nil type.
    ///
    /// # Examples
    ///
    /// ```
    /// # use hadron::runtime::slot::{Slot, SlotType};
    /// let n = Slot::nil();
    /// let c = Slot::from_character('\0');
    /// assert!(n.is_nil());
    /// assert!(!c.is_nil());
    /// ```
    pub fn is_nil(&self) -> bool {
        self.bits == Slot::NIL_TAG
    }
}

impl Default for Slot {
    fn default() -> Self { Slot::nil() }
}

impl std::fmt::Debug for Slot {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.extract().fmt(formatter)
    }
}

impl std::fmt::Debug for SlotType {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            SlotType::Float(f) => write!(formatter, "Float: {}", f),
            SlotType::Integer(i) => write!(formatter, "Integer: {}", i),
            SlotType::Boolean(b) => write!(formatter, "Boolean: {}", b),
            SlotType::Character(c) => write!(formatter, "Character: '{}'", c),
            SlotType::Nil => write!(formatter, "nil"),
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::runtime::slot::Slot;

    #[test]
    fn bool_simple() {
        let boolean_true = Slot::from_boolean(true);
        let boolean_false = Slot::from_boolean(false);
        assert!(boolean_true.is_boolean());
        assert!(boolean_false.is_boolean());
        assert_ne!(boolean_true, boolean_false);
        assert!(boolean_true
            .as_boolean()
            .expect("boolean true should return a valid bool"));
        assert!(!boolean_false
            .as_boolean()
            .expect("boolean false should return a valid bool"));
    }

    #[test]
    fn character_simple() {
        let character_a = Slot::from_character('a');
        assert!(character_a.is_character());
        assert_eq!(
            character_a
                .as_character()
                .expect("character_a should be a valid character"),
            'a'
        );

        let character_a_upper = Slot::from_character('A');
        assert_ne!(character_a, character_a_upper);
    }

    #[test]
    fn character_utf8() {
        let character_cat = Slot::from_character('😼');
        assert!(character_cat.is_character());
        assert_eq!(
            character_cat
                .as_character()
                .expect("character_cat should return a valid character"),
            '😼'
        );

        let character_dog = Slot::from_character('🐶');
        assert_ne!(character_cat, character_dog);
    }

    #[test]
    fn float_simple() {
        let float_positive = Slot::from_float(1.1);
        assert!(float_positive.is_float());
        assert_eq!(
            float_positive
                .as_float()
                .expect("float_positive should return a valid float"),
            1.1
        );

        let float_zero = Slot::from_float(0.0);
        assert!(float_zero.is_float());
        assert_eq!(
            float_zero
                .as_float()
                .expect("float_zero should return a valid float"),
            0.0
        );

        let float_negative = Slot::from_float(-2.2);
        assert!(float_negative.is_float());
        assert_eq!(
            float_negative
                .as_float()
                .expect("float_negative should return a valid float"),
            -2.2
        );
        assert_ne!(float_negative, float_positive);
    }

    #[test]
    fn float_min_max() {
        let float_min = Slot::from_float(f64::MIN);
        assert!(float_min.is_float());
        assert_eq!(
            float_min
                .as_float()
                .expect("float_min should return a valid float"),
            f64::MIN
        );

        let float_max = Slot::from_float(f64::MAX);
        assert!(float_max.is_float());
        assert_eq!(
            float_max
                .as_float()
                .expect("float_max should return a valid float"),
            f64::MAX
        );

        assert_ne!(float_min, float_max);
    }

    #[test]
    fn float_nan() {
        let float_nan = Slot::from_float(f64::NAN);
        assert!(float_nan.is_float());
        assert!(float_nan
            .as_float()
            .expect("nan stored as a float should be a float")
            .is_nan());
    }

    #[test]
    fn float_inf() {
        let float_inf = Slot::from_float(f64::INFINITY);
        assert!(float_inf.is_float());
        let inf = float_inf
            .as_float()
            .expect("infinity stored as a float should be a float");
        assert_eq!(inf, f64::INFINITY);

        let float_negative_inf = Slot::from_float(f64::NEG_INFINITY);
        assert!(float_negative_inf.is_float());
        let negative_inf = float_negative_inf
            .as_float()
            .expect("negative infinity stored as a float should be a float");
        assert_eq!(negative_inf, f64::NEG_INFINITY);

        assert_ne!(float_inf, float_negative_inf);
    }

    #[test]
    fn float_equality() {
        let f1 = Slot::from_float(25.0);
        let f2 = Slot::from_float(25.1);
        let f3: Slot = Slot::from_float(25.0);
        assert_eq!(f1, f1);
        assert_ne!(f1, f2);
        assert_eq!(f1, f3);
        assert_ne!(f2, f3);
        assert_ne!(f1, Slot::from_integer(25));
    }

    #[test]
    fn integer_simple() {
        let int_positive = Slot::from_integer(1);
        assert!(int_positive.is_integer());
        assert_eq!(int_positive.as_integer().expect("pos"), 1);

        let int_zero = Slot::from_integer(0);
        assert!(!int_zero.is_nil());
        assert!(int_zero.is_integer());
        assert_eq!(int_zero.as_integer().expect("zero"), 0);

        let int_negative = Slot::from_integer(-1);
        assert!(int_negative.is_integer());
        assert_eq!(int_negative.as_integer().expect("neg"), -1);
    }

    #[test]
    fn integer_min_max() {
        let integer_min = Slot::from_integer(i32::MIN);
        assert!(integer_min.is_integer());
        assert_eq!(integer_min.as_integer().expect("min int"), i32::MIN);

        let integer_max = Slot::from_integer(i32::MAX);
        assert!(integer_max.is_integer());
        assert_eq!(
            integer_max
                .as_integer()
                .expect("integer_max should be an integer"),
            i32::MAX
        );

        assert_ne!(integer_min, integer_max);
    }

    #[test]
    fn integer_equality() {
        let i1 = Slot::from_integer(-22);
        let i2 = Slot::from_integer(-23);
        let i3 = Slot::from_integer(-22);
        assert_eq!(i1, i1);
        assert_ne!(i1, i2);
        assert_eq!(i1, i3);
        assert_ne!(i2, i3);
    }

    #[test]
    fn nil() {
        let nil = Slot::nil();
        assert_eq!(nil, Slot::nil());
        assert_ne!(nil, Slot::from_boolean(false));
        assert_ne!(nil, Slot::from_character('\0'));
        assert_ne!(nil, Slot::from_float(0.0));
        assert_ne!(nil, Slot::from_float(f64::NAN));
        assert_ne!(nil, Slot::from_integer(0));
    }
}
