# Methodology: what "matching" means and how evidence is weighed

Byte identity with the original is how a reconstruction is *tested*; the goal is the original source text, as far as
the evidence can recover it. This page defines the test, the rules on what source is acceptable, and how evidence is
ranked when bytes alone do not decide. The build pipeline is in [BUILD-PIPELINE.md](BUILD-PIPELINE.md); the classes of
measurement-chosen text are in [DECISIONS.md](DECISIONS.md).

## What "Matching" means

A source file (a translation unit, normally one `.c` file) is marked `Matching` in [configure.py](../configure.py) only
when **everything it owns** in the executable is reproduced: its code, its read-only constants and string literals,
its initialised data, the layout of its zero-initialised data (every BSS variable's offset, size and alignment), and
the padding between its pieces. There is no per-function selection: a file with 49 identical functions and one that is
a byte short is not matching, and its bytes still come from the original. Each version is checked against its own
executable: a file is Matching for 1.2 when its 1.2 text passes against the 1.2 image, and for 1.0 when its 1.0 text
(the `versions/1.0/` copy, where there is one) passes against the 1.0 image.

The check is strict:

1. All sections a matching file compiles to are placed at their reviewed retail addresses; symbol offsets, sizes and
   alignment are checked.
2. The i386 relocations are resolved and the complete `.text`, `.rdata` and `.data` contents compared. BSS must match
   its full storage layout.
3. Missing definitions, compiled sections nobody accounted for, unexplained gaps and extents that do not agree are
   failures, never skipped.
4. A matching file supplies all of its owned bytes to the rebuilt executable; it cannot silently fall back to the
   original after a regression.
5. The rebuilt executable must have the original's complete SHA-256 and SHA-1.

A passing build therefore proves that every matching file is right. It does not by itself prove that the whole game has
been decompiled: a file that does not match takes its bytes from the original, and some library code and unused bytes
are copied from the retail executable. [BYTE-BREAKDOWN.md](BYTE-BREAKDOWN.md) shows where every byte comes from.

### Proxies, ranked

Before a file matches, progress is measured with proxies, from weakest to strongest:

1. **objdiff similarity** (the per-function percentages in `report.json`): it aligns instruction rows and gives partial
   credit for wrong code.
2. **Relocation-masked differing bytes**, together with agreement with retail's function length.
3. **Byte-exact functions.**

A measured change is kept only when differing bytes go down, the function's length equals retail's, and its
instruction count equals retail's or moves towards it. A body 16 bytes short or long is wrong whatever its byte count
says, and an equal length at a different instruction count is a coincidence, not a found shape.

## What is not allowed

The source must be source a 1999 programmer could have written, producing the bytes because it is the right text, not
because it was bent until it did. None of these is used to force a match:

- `register` or `volatile` to steer register allocation or ordering;
- `goto` or labels, inline assembly, raw structure offsets, pointer arithmetic on integer addresses, artificial stack
  layouts, instruction byte arrays;
- dummy operations, dead branches, artificial padding, `do { } while (0)` wrappers;
- casts, parentheses or temporaries kept only because a score rose, outside the labelled, rule-bound classes of
  [DECISIONS.md](DECISIONS.md);
- patching compiled objects, substituting original bytes, or reordering output after the compiler.

`goto`, inline assembly and `volatile` do occur at a closed list of sites where the original evidently used them; each
site is named, limited to its functions and enforced by a test, and its evidence is given in
[DECISIONS.md](DECISIONS.md#9-the-listed-exceptions-goto-inline-assembly-volatile). Statements and locals that the
debug build shows but release compiles to nothing (an unused local, a discarded call result) are evidence, not dummy
operations, and are kept.

## How evidence is ranked

When two spellings compile to the same bytes, or a number and the source evidence disagree, evidence decides. In order:

1. **Immediates at call sites.** A `push <imm32>` of a message or `__LINE__` value is a statement at that site; a
   `push <reg>` is a value carried there from elsewhere.
2. **Frame size.** Retail's `sub esp,N` against the exact size of the function's locals: if they already account for
   `N`, a source with more locals is wrong. This is the most reliable discriminator.
3. **Original text.** Assertion expressions, diagnostic messages, `__FILE__` paths, parser-generator strings and
   tables, and the unoptimised debug build of the same source ([DEBUG-BUILD.md](DEBUG-BUILD.md)).
4. **Instruction counts**, read alongside byte counts.
5. **Removing a construct that never had evidence.** A parenthesis, cast, temporary, flag or label kept only because it
   improved a score is removed, even when the number falls.
6. **One text written many times.** A macro expansion or a uniform authoring habit is judged across the whole project,
   with every function that gets worse explained, never site by site.

A hunch, a tidier source or "consistency with a sibling" is not evidence.

**Stepping backwards is allowed.** A change the evidence demands is kept even when a measure gets worse, even when a
matching file stops matching; the cost is recorded beside the change and the file is matched again later. A loss of
working source with no evidence behind it is always rejected.

## Release bytes and the debug build

Retail is optimised, so many texts compile to the same release bytes. The Italian release's debug build compiles the
same source without optimisation or inline expansion and shows much of what the release build hides: statement order,
control flow, the number and types of locals, macros against helpers, shared error blocks. Every function is therefore
checked twice: the release build must be byte-identical to retail, and the same source compiled the way the debug
build was (`/Od /Ob0`) must be identical to the debug image, or identical apart from which stack slot each local
occupies. What the debug build cannot show (parentheses, declaration order, names) is listed in
[DEBUG-BUILD.md](DEBUG-BUILD.md#what-it-can-and-cannot-prove); how the project chooses those, and labels each choice,
is in [DECISIONS.md](DECISIONS.md).
