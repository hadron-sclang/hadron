enum State {
    TopLevelStatement,


    ClassDefStart,

    ClassExtDef,
    InterpreterCode,
//    ClassVarDecl,
}

struct StateStackEntry {
   pub state: State,
   pub token_index: i32,
   pub subtree_start: i32,
}

pub struct Context<'a> {
    states: Vec<StateStackEntry>,
}