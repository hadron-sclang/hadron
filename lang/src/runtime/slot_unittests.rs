#[cfg(test)]
mod tests {
    use crate::runtime::slot::{Slot, SlotType};

    #[test]
    fn insert_and_extract() {
        let b = Slot::insert(SlotType::Boolean(false));
        assert_eq!(b.extract(), SlotType::Boolean(false));

        let c = Slot::insert(SlotType::Character('$'));
        assert_eq!(c.extract(), SlotType::Character('$'));

        let f = Slot::insert(SlotType::Float(-1.0));
        assert_eq!(f.extract(), SlotType::Float(-1.0));

        let i = Slot::insert(SlotType::Integer(2));
        assert_eq!(i.extract(), SlotType::Integer(2));

        let n = Slot::insert(SlotType::Nil);
        assert_eq!(n.extract(), SlotType::Nil);
    }

    #[test]
    fn bool_simple() {
        let boolean_true = Slot::from_boolean(true);
        let boolean_false = Slot::from_boolean(false);
        assert!(boolean_true.is_boolean());
        assert!(boolean_false.is_boolean());
        assert_ne!(boolean_true, boolean_false);
        assert!(boolean_true.as_boolean().expect("boolean true should return a valid bool"));
        assert!(!boolean_false.as_boolean().expect("boolean false should return a valid bool"));
    }

    #[test]
    fn character_simple() {
        let character_a = Slot::from_character('a');
        assert!(character_a.is_character());
        assert_eq!(
            character_a.as_character().expect("character_a should be a valid character"),
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
            character_cat.as_character().expect("character_cat should return a valid character"),
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
            float_positive.as_float().expect("float_positive should return a valid float"),
            1.1
        );

        let float_zero = Slot::from_float(0.0);
        assert!(float_zero.is_float());
        assert_eq!(float_zero.as_float().expect("float_zero should return a valid float"), 0.0);

        let float_negative = Slot::from_float(-2.2);
        assert!(float_negative.is_float());
        assert_eq!(
            float_negative.as_float().expect("float_negative should return a valid float"),
            -2.2
        );
        assert_ne!(float_negative, float_positive);
    }

    #[test]
    fn float_min_max() {
        let float_min = Slot::from_float(f64::MIN);
        assert!(float_min.is_float());
        assert_eq!(float_min.as_float().expect("float_min should return a valid float"), f64::MIN);

        let float_max = Slot::from_float(f64::MAX);
        assert!(float_max.is_float());
        assert_eq!(float_max.as_float().expect("float_max should return a valid float"), f64::MAX);

        assert_ne!(float_min, float_max);
    }

    #[test]
    fn float_nan() {
        let float_nan = Slot::from_float(f64::NAN);
        assert!(float_nan.is_float());
        assert!(float_nan.as_float().expect("nan stored as a float should be a float").is_nan());
    }

    #[test]
    fn float_inf() {
        let float_inf = Slot::from_float(f64::INFINITY);
        assert!(float_inf.is_float());
        let inf = float_inf.as_float().expect("infinity stored as a float should be a float");
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
        assert_eq!(integer_max.as_integer().expect("integer_max should be an integer"), i32::MAX);

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
