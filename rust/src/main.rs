#![allow(unused)]

use crate::value::Numeral;
use squire::value::Codex;
use squire::*;
use squire::value::Array;

fn main() {
    let mut stream = parse::Stream::from_str(r#""a\(yay + 4)[\n]\(34)!", world"#);
    let mut tokenizer = parse::Tokenizer::new(&mut stream);

    dbg!(tokenizer.next());
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
