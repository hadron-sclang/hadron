pub struct Node<'a> {
    pub kind: NodeKind,
    pub token_index: i32,
    pub subtree_size: i32,
}

// A parse tree.
pub struct Tree<'a> {
    tokens: Vec<Token>,
    nodes: Vec<Node>,
}

impl<'a> Tree<'a> {
    pub fn new() -> Tree<'a> {
        Tree { nodes: Vec<Node>::new() }
    }

    pub fn size(&self) -> usize {
        self.nodes.size()
    }
}
