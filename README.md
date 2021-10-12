# Squire
Hear ye hear ye! Thou are commandeth to partake in the most novel of programming parlances in this era: Squire. It doth contain a truly awful cohort of subliminal mechanisms for your writing pleasure.

Squire's a medieval-themed programming language that's meant to parody most mainstream programming languages. While it does definitely contain its fair share of fun quirks, it's first and foremost meant to be a fully-fledged language, completely oblivious to its own ridiculousness.

# Examples
Here's some examples of Squire. For more examples, you can see [examples](/examples), and for a more complete overview of Squire itself, see [overview](/overview)

## Basic Examples
Squire uses fun, old-timey words as alternatives to modern-day concepts:
```squire
i = 0; # semicolons are optional, but recommended
whilst i < 10 { # no parens needed
	if i % 2 { # squire uses truthy values
		proclaim("{i} is odd"); # string interpolation
	} alas {
		proclaim("{i} is even");
	}
	i += 1;
}
```

### Journeys (ie functions)
Instead of functions, you go on journeys (and get `reward`ed for `return`ing)
```squire
journey greet(what) {
	reward "Hello, {what}";
}

proclaim(greet("world"));
```

Because of poor design choices early on, you have to predeclare functions used later on as global variables
```squire
journey foo(x) {
	renowned bar; # `bar` is a global variable in this scope.
	reward "Hello, " + bar(x)
}

journey bar(x) {
	reward x + "!";
}

proclaim(foo("world")); # Hello, world!
```

### Forms (ie classes)
The medieval people were _really_ into Plato's forms, so I decided to name Squire's "Classes" after it. (And I majored in Ancient Philosophy):

```squire
form Person {
	matter name : Text; # type annotations are optional
	matter hungry;

	imitate(name) { # constructor
		soul.name = name; # `soul` is `this`/`self`.
		soul.hungry = nay;
	}

	change walk(miles) {
		if soul.hungry {
			proclaim("I'm too hungry to walk.");
		} alas {
			proclaim("I walked {miles} miles!");
			soul.hungry = yea;
		}
	}
}

me = Person("Sam");
me.walk(5); # I walked V miles! (more on `V` in the fun quirks section)
me.walk(5); # I'm too hungry to walk.
```

# Fun quirks of Squire!
Here's a (non-exhaustive) list of Squire's quirks. Most of these things are actually encouraged (especially the [Roman Numeral literals](#roman-numeral-literals) and [Fraktur bare word literals](#fraktur-bare-word-literals)) when possible, as they add to the charm of the language.

## Roman Numeral Literals
Back in the medieval ages, people actually did all their mathematical computations with Roman numerals. As such, Squire supports (and encourages) using roman numeral literals:
```squire
proclaim("twenty four is: {IV * VI}") # => twenty four is: XXIV
```
In fact, all number to string conversions (regardless of whether they were written in Roman or Arabic numerals) will output Roman numerals. If you want Arabic numerals, you have to use the `arabic` function:
```
proclaim("twenty four is: {arabic(IV * VI)}") # => twenty four is: 24
```
Note: You can enable the `SQ_NUMERAL_TO_ARABIC` flag to switch the default string output to Arabic numerals. (Though this may be deprecated/removed.)

## `yea`/`nay`/`ni`
Squire supports the basic concepts of true and false. However, since Mr. Boole wasn't born until 1815, the concepts of Boolean algebra did not exist. So instead we use the phrases `yea` and `nay`.

The word `ni` is used for null, and is a nod to the skit "Monte Python and the Holy Grail."

## Fraktur Bare word literals
Squire supports bare words---that is, strings that don't require quotes around them. Now, normally this is a _terrible_ idea, as there's no way to determine if a word is a variable or a string just by looking at it. Squire solves this problem by requiring all bare words to use [Fraktur Unicode characters](https://en.wikipedia.org/wiki/Fraktur#Fraktur_in_Unicode) exclusively. Note that the bare words can also contain spaces in them, and all leading and trailing whitespace will be trimmed.

```
proclaim(ℌ𝔢𝔩𝔩𝔬 𝔴𝔬𝔯𝔩𝔡) # => Hello world

# You can also escape characters that'd normally end the fraktur literal:
proclaim(ℌ𝔢𝔩𝔩𝔬\, 𝔴𝔬𝔯𝔩𝔡\!) # => Hello, world!
```

The Fraktur characters will be converted to their ASCII equivalents within the parser, so the actual Fraktur characters are never encountered within Squire (unless you write them in quotes).

## "Coding Case"-Insensitivity
Back in the medieval ages, spelling was not at all standardized—a lot of words simply would be spelled differently from region to region, and sometimes [things got really bad](https://en.wikipedia.org/wiki/Ough_%28orthography%29). Squire embodies this by allowing you to have "coding case"-insensitive identifiers. That is, you can use both `snake_case` and `camelCase` and Squire will interpret them as the same name! In fact, you can also use `kebab-case` and `space case` too :-)

These are all equivalent:
```
foo_bar_baz
fooBarBaz
foo-bar-baz
foo bar baz
fooBar_baz
foo-bar baz
```

The only caveat to this is if the variable starts with a capital letter, this automatic replacement is not performed. 

## Were-`if`s
What programming language based on the medieval ages would be complete without some form of monsters? In Squire, instead of having werewolfs, we have were-`if`s: On full moons (and only on full moons), `if` statements have a 1% chance of executing their body if the condition is _false_. That is this may be executed on a full moon:
```squire
if nay {
	proclaim("'Ello world!")
}
```
What more, this will only occur when the moon is 95% full or greater. This means that certain parts of the world may have were-`if`s occurring where other parts won't. Happy bug hunting! (Note that you _can_ disable this by passing `-DSQ_NMOON_JOKE` when compiling, but that's no fun.)

And yes, I did embed a [moon phase calculator](https://github.com/sampersand/squire/blob/master/src/value/moon-phase.c) for a joke. No I do not feel ashamed.

## `whence`
As you may have noticed, Squire does not actually have keywords for `break` or `continue`: This is because when I was originally throwing together the language, I wanted to have a one-pass compiler, which doesn't play nicely with `break` and `continue`. So I simply omitted them. The obvious problem is that you can't break out of nested loops easily.

Enter `whence`. A `whence <label>` statement is akin to the [`COMEFROM` statement](https://en.wikipedia.org/wiki/COMEFROM) in other languages: Whenever `<label>` is encountered, control flow is actually diverted to the `whence label`. As such, you can write loops like such:
```
i = I;
whilst yea {
	if i > 100 {
	done:
		# normally you'd `break` here
	}
	i = i + I
}

whence done; # whenever `done:` is hit, we go here.
```

That leaves one problem: What happens when multiple `whence`s all point to the same label:
```
foo:
proclaim("1");

whence foo;
proclaim("2");

whence foo;
proclaim("3");
```
Squire will actually execute all the `whence`s by simply `fork`ing for every `whence` statement but the last one. So the output of the above program would be either `233` or `323`, depending on how the OS scheduled the two processes.

## Macros
Yes, we have macros.
# Actually useful things
## Pattern Matching
## `path`'s body
## Typing

# FAQs
## Why?
Why not? 

## No seriously, why?
I originally was working on a language called [Quest](https://github.com/sampersand/quest). I had just written a very simple programming language _within_ Quest (the precursor to [Knight](https://github.com/knight-lang/knight-lang)), and I wanted to write something a bit more complex. I finished a rough sketch of syntax, but never ended up finishing it.

Fast forward a few years, when I wanted to write a programming language that's a bit more complex than Knight, but not as complex as Quest. The original Squire looked very much like Go or JavaScript, as it was meant to simply be another toy language. But then it hit me: I could make this a parody, and have it be medieval themed (like the naming scheme of my projects). And thus, Squire was born.

## Why is <code snippet> so poorly written?
When I first was writing Squire, I never intended for it to become anything. In fact, I purposefully wrote it late at night when I was tired/watching TV, and _never_ went back to fix anything. Only much later did I start taking the project a bit more seriously and began writing cleaner code. So the bad code you'll see is from those early stages (most notably the tokenizer, parser, and compiler.)

# Possible additions:

- [x] Static fields on `form`s
- [x] Inheritence for `form`s
- [x] macros
	- [x] Declarative
	- [x] Function
	- [~] conditional compilation (could be cleaned up)
	- [ ] `foreach`
	- [ ] `evaluate`
- [x] pattern matching
- [~] varidict functions and keyword parameters
	- varidict is done, keyword is not
- [ ] modules
- [x] dictionaries
- [x] interpolation
- [ ] overload
- [x] pattern matching on functions

<!-- 
# Glossary
Squire doth contain a multitude of fancy parlance:
- `tome`s are dictionaries. -->