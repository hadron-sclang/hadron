#[cfg(test)]
mod tests {
    use crate::sclang;
    use crate::toolchain::parser::node::{ClassDefKind, Node, NodeKind::*};
    use crate::toolchain::parser::tree_unittests::tests::check_parsing;
    use crate::toolchain::source;

    #[test]
    fn test_bare_class() {
        check_parsing(
            sclang!("A {} B {}"),
            vec![
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 2,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 7,
                    subtree_size: 1,
                    closing_token: Some(8),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 5,
                    subtree_size: 2,
                    closing_token: Some(8),
                    has_error: false,
                },
            ],
            vec![],
        );
    }

    #[test]
    fn test_storage_type() {
        check_parsing(
            sclang!("A[float]{}"),
            vec![
                Node {
                    kind: Name,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::ArrayStorageType },
                    token_index: 1,
                    subtree_size: 2,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 4,
                    subtree_size: 1,
                    closing_token: Some(5),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 4,
                    closing_token: Some(5),
                    has_error: false,
                },
            ],
            vec![],
        );
    }

    #[test]
    fn test_superclass() {
        check_parsing(
            sclang!("A:B{}"),
            vec![
                Node {
                    kind: Name,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Superclass },
                    token_index: 1,
                    subtree_size: 2,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 3,
                    subtree_size: 1,
                    closing_token: Some(4),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 4,
                    closing_token: Some(4),
                    has_error: false,
                },
            ],
            vec![],
        )
    }
}
