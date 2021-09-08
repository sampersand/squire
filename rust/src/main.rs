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
			Ok("W") | Ok("WARN") | Err(_) => LevelFilter::WARN,
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
	let input = std::env::args().skip(2).next().unwrap_or(DEFAULT.to_string());

	let mut stream = parse::Stream::from_str(&input);
	let mut tokenizer = parse::Tokenizer::new(&mut stream);
	let mut parser = parse::Parser::new(&mut tokenizer);
	let mut compiler = compile::Compiler::default();
	compiler.compile_with(&mut parser).unwrap();
	let (block, mut vm) = compiler.finish_with_vm();

	block.run(Default::default(), &mut vm).unwrap();
}

const DEFAULT: &str = r##"

journey say-fizzbuzz
    (n) if !(n % XV)  = proclaim(𝔉𝔦𝔵𝔵𝔅𝔲𝔵𝔵),
    (n) if !(n % III) = proclaim(𝔉𝔦𝔵𝔵),
    (n) if !(n % V)   = proclaim(𝔅𝔲𝔵𝔵),
    (n)               = proclaim(n);

@__END__
i = 0;

whence foo;
proclaim("a");

if i != 10 {
    i = i + 1;
    thence foo;
    proclaim("b");
    foo:
}

proclaim("c");
@__END__```
```
a
a
a
a
a
a
a
a
a
a
a
c```
// "while loop"
whence end;

@__END__
i = 0;

whence foo;
proclaim(i);

if i != 3 {
   i = i + 1;
   thence foo;
   proclaim("b");
   foo:
}

proclaim(i);

#if nay { whence foo; }

@__END__
form Foo {
   matter a;
   essence b;

   imitate (c) {
	  renowned Foo;
	  soul.a = c;
	  Foo.b = 4;
   }

   change d(e) {
	  proclaim("e=");
	  proclaim(e);
   }

   change to-text() {
	  reward "a" + soul.a;
   }
};

f = Foo(34);
f.d(45);
proclaim(f);
proclaim(Foo.b);
@__END__
form f { 
	essence x=3;
	recall y() { 34 }
	change z(l) { soul.a + l }
	change q(l) { soul.a * l }
}

form g : f {
	matter a;
	change w(){ 56 }
	change q(){ 78 }
}

l = g(1)
dump(g.x); # 3
dump(g.y()); # 34
dump(l.z(1)); # 2
dump(l.w()); # 56
dump(l.q()); # 78



@__END__
x=[1];
x[1]=50;
proclaim(x,x,x);
__END__
form Foo {
	matter a, b;
	essence x;
	recall bar() { dump(34) }
	imitate () { my.a = 3; my.b = 4 }
	change lol() { my.a + my.b }
}
#Foo.bar();
#dump(Foo(1, 2));
dump(Foo());
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
"##;