#![allow(unused)]

use squire::*;
use squire::parse::Parsable;
use squire::compile::Compilable;


fn setup_tracing() {
    use tracing::level_filters::LevelFilter;
    use tracing_subscriber::{layer::SubscriberExt, registry::Registry};
    use tracing_tree::HierarchicalLayer;

    let loglevel = std::env::var("SQUIRE_LOGGING");
    let filter = 
        match loglevel.as_ref().map(|x| x.as_ref()) {
            Ok("T") | Ok("TRACE") => LevelFilter::TRACE,
            Ok("D") | Ok("DEBUG") => LevelFilter::DEBUG,
            Ok("I") | Ok("INFO") => LevelFilter::INFO,
            Ok("W") | Ok("WARN") => LevelFilter::WARN,
            Ok("E") | Ok("ERROR") => LevelFilter::ERROR,
            Ok("O") | Ok("OFF") => LevelFilter::OFF,
            Ok("TREE") => {
                let layer = HierarchicalLayer::default()
                    .with_indent_lines(true)
                    .with_indent_amount(2)
                    .with_thread_names(true)
                    .with_thread_ids(true)
                    .with_verbose_exit(true)
                    .with_verbose_entry(true)
                    .with_targets(true);

                let subscriber = Registry::default().with(layer);
                tracing::subscriber::set_global_default(subscriber).unwrap();
                return;
            },
            _ => return
        };

    tracing_subscriber::fmt()
        .with_max_level(filter)
        .with_span_events(tracing_subscriber::fmt::format::FmtSpan::FULL)
        .init();
}

fn main() {
    setup_tracing();
//     // let arg = std::env::args().skip(1).next().unwrap();
//     let mut stream = parse::Stream::from_str(&arg/*r#"
// forma
// # @henceforth $foo=34
// #       "a\(yay + 4)[\]\(34)!", world
//     "#*/);
    let mut stream = parse::Stream::from_str(r##"
form Foo { recall bar() {} }
Foo.bar()
__END__
fork 4 {
    path 1:
    path 2:
        dump(34);
    path 3:
        dump(45);
        dump(56)
}
dump(67);
__END__
attempt {
    #catapult "!"
    dump(1 / 0);
} alas name {
    dump(name)
}
__END__
renowned foo = 34;
journey square(a) {
    renowned foo;
    dump(foo);
    foo = 45;
    { [34]: a }
}

dump(square(4)[[a=34]]);
dump(foo);
__END__
#if yay{} alas{}
#a=1;
#if yay { dump(3) } alas { dump(4) } 
i=1;
whilst (i < 10) {
    dump(i);
    i = i + 1;
}
__END__
if yay { dump(3) } alas { dump(4) } 
dump(5)
__END__
a=1;
dump(-a + 4);
#[3 + 4, 5 - 9 - 2]
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
    let mut compiler = compile::Compiler::default();
    compiler.compile_with(&mut parser).unwrap();
    let (block, mut vm) = compiler.finish_with_vm();

    block.run(Default::default(), &mut vm).unwrap();
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
