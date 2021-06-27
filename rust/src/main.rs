#![allow(unused)]

use squire::*;
use squire::parse::Parsable;
use squire::compile::Compilable;

fn main() {
//     // let arg = std::env::args().skip(1).next().unwrap();
//     let mut stream = parse::Stream::from_str(&arg/*r#"
// forma
// # @henceforth $foo=34
// #       "a\(yay + 4)[\]\(34)!", world
//     "#*/);
    let mut stream = parse::Stream::from_str(r##"
-34
#journey x(a, b=3, c: int, d: (int; 34; int) = 3, *e, **f){}
__END__
fork "A" {
    path "A":
    path "B":
    path yay:
        if 123 { 456 }
        alas {789}
    alas:
        ;;34;
}

#whilst 123 { 3 ; if 4 { 5 } } alas { 456 }

# 
# form Foo {
#     matter x, y;
#     essence a, c;
#     essence b = 3;
# }
"##);
    let mut tokenizer = parse::Tokenizer::new(&mut stream);
    let mut parser = parse::Parser::new(&mut tokenizer);

    let parsed = ast::Expression::parse(&mut parser).unwrap().unwrap();
    let mut compiler = compile::Compiler::default();

    let index = compiler.next_target();
    parsed.compile(&mut compiler, Some(index)).unwrap();
    dbg!(compiler);
    // dbg!(tokenizer);
/*
pub enum Keyword {
    Class,
    Method,
    Field,
    ClassField,
    ClassFn,
    Constructor,
    Function,

    Global,
    Local,

    If,
    Else,
    ComeFrom,
    While,
    Return,
    Try,
    Catch,
    Throw,
    Switch,
    Case,
    Assert,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ParenKind {
    Round,
    Square,
    Curly
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Symbol {
    Endline,
    Comma,
    Colon,
    Dot,
    Equal,

    EqualEqual,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Compare,

    Plus,
    PlusEqual,
    Hyphen,
    HyphenEqual,
    Asterisk,
    AsteriskEqual,
    Solidus,
    SolidusEqual,
    PercentSign,
    PercentSignEqual,

    Exclamation,
    AndAnd,
    OrOr
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Literal {
    Null,
    Boolean(bool),
    Numeral(Numeral),
    Text(Text), // possibly with interpolation
    TextInterpolation(Vec<(Text, Vec<Token>)>, Text),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Token {
    Keyword(Keyword),
    Symbol(Symbol),
    LeftParen(ParenKind),
    RightParen(ParenKind),
    Literal(Literal),
    StringInterpolation(Vec<(String, Vec<Token>)>, String),
    Identifier(String),
    Label(String),
}


*/
}

//     // dbg!(stream.take_identifier("12"));
//     // dbg!(stream.next());
//     // dbg!(stream.next());
//     // dbg!(stream.next());
//     // dbg!(stream.next());
//     // dbg!(stream.next());
//     // dbg!(stream.next());
//     println!("{:#}", Numeral::new(12346).display_roman());
//     // let mut codex: Codex = vec![(true.into(), true.into()), (false.into(), false.into())].into_iter().collect();

//     // println!("Hello, world! {:?}", codex);
//     // codex.merge([(Value::Null, Value::Null)]);
//     // println!("Hello, world! {:?}", codex);
// }
