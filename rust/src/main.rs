#![allow(unused)]

use squire::value::Codex;
use squire::*;
use squire::value::Array;

fn main() {
    let mut stream = parse::Stream::from_str("123, world");

    dbg!(stream.take_identifier("12"));
    dbg!(stream.next());
    dbg!(stream.next());
    dbg!(stream.next());
    dbg!(stream.next());
    dbg!(stream.next());
    dbg!(stream.next());
    // let mut codex: Codex = vec![(true.into(), true.into()), (false.into(), false.into())].into_iter().collect();

    // println!("Hello, world! {:?}", codex);
    // codex.merge([(Value::Null, Value::Null)]);
    // println!("Hello, world! {:?}", codex);
}
