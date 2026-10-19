# Inside Visual C++ 5.0's code generator: selected findings

Matching the last bytes of a function often depends on a decision deep inside the compiler. To make those decisions
predictable, the project traced the Visual C++ 5.0 SP3 back end (`c2.exe` 11.00.7303) while it compiled small test
files and real game files: breakpoints on the routines that make each decision, with their inputs logged. This
document collects the most useful results. Each one is stated as a rule and tied to a game function that shows it.
Addresses such as `0x408d58` are code addresses inside `c2.exe` 11.00.7303.

Two principles apply throughout:

- A diagnostic change can show *how big* a gap is, for example a dummy statement that makes a function identical.
  That is not evidence of what the original source was, and such changes never stay in the source.
- Every rule below is stated with the conditions under which it was measured. A rule that seems to contradict another
  usually has a condition nobody wrote down.

## Contents

1. [A pointer returned by a call may point at any address-taken local in scope](#1-a-pointer-returned-by-a-call-may-point-at-any-address-taken-local-in-scope)
2. [Loop-invariant code motion, and the re-test that stops it](#2-loop-invariant-code-motion-and-the-re-test-that-stops-it)
3. [Commutative operands are sorted by a key](#3-commutative-operands-are-sorted-by-a-key)
4. [Which floats stay on the x87 stack](#4-which-floats-stay-on-the-x87-stack)
5. [Tail merging happens once, after register allocation](#5-tail-merging-happens-once-after-register-allocation)
6. [The number of declarations before a function changes its code](#6-the-number-of-declarations-before-a-function-changes-its-code)
7. [Parentheses are code](#7-parentheses-are-code)
8. [Constant flags are threaded only in simple shapes](#8-constant-flags-are-threaded-only-in-simple-shapes)
9. [The host does not matter for Microsoft's compilers](#9-the-host-does-not-matter-for-microsofts-compilers)

## 1. A pointer returned by a call may point at any address-taken local in scope

**Rule.** c2's memory-dependence test (`0x407fa0`) decides whether a load may be moved past a store:

- A local whose address is never taken does not alias anything.
- An address-taken local aliases only a reference whose points-to set names it.
- Pointers that are parameters, globals or loaded from memory point to "unknown". "Unknown" reaches globals and
  statics, never locals.
- A call aliases everything.
- A **pointer returned by a call** may point at any address-taken local **that is in scope at that call**.

So the scope of an address-taken local is visible in optimised code even though `/Od` code does not show it.

**Witness: `sithWeapon_GetAimOrient` (`0x00448DB0`).** The function calls a search that returns `pTarget`, then copies
vectors into three address-taken locals (`targetPos`, `vecTargetFire`, `zOffset`). Declared at function scope, the locals
are in scope at the call. The copy into `targetPos` then counts as a possible store through `pTarget`, and that kills
the value of `flags`, which retail reuses across the copy. Declared inside `if ( pTarget )`, after the call, they are out of its
reach, and the debug build's text compiles to retail's bytes. `sithShadow_SearchForAttachDistance` (`0x004B3E70`) and
`sithCogFunctionThing_RotatePivot` became exact the same way.

A second consequence: an indexed global array (`aTable[i].f`) points only to itself, so its reads are never ordered
against stores to locals, under any spelling.

## 2. Loop-invariant code motion, and the re-test that stops it

**Rule.** Loop-invariant code motion (`0x43ada6`) runs once per block of each loop. It hoists an invariant computation,
typically an address `p + d` passed as `&p->f`, into the loop's preheader **when its block dominates every exit of the
loop**. The loop header always does. Such an address stays inside the loop only if one of two things holds:

- an exit precedes it in the loop at the time this pass runs;
- the base pointer is assigned inside the loop.

There is no third way. The pass runs before the compiler folds tests whose outcome an identical earlier test already
decided. It also runs before tests on a variable that the loop re-assigns to its entry value are folded. A test of either
kind is still an exit while the pass runs, and it is folded away afterwards without leaving a byte.

**Witness: `rdPuppet_AdvanceTrack` (`0x004CCBA0`).** Its three loops are written as

```c
if ( pTrack->pKFTrack )
{
    ...
    for ( i = 0; pTrack->pKFTrack && i < numMarkers; ++i )
```

The loop condition re-tests the guard. Deleting the re-test makes the compiler hoist seven field addresses (the
callback's arguments) out of the loop, and the function goes from identical to 398 differing bytes. So the original
authors re-tested a guard at the head of a loop, and that habit is what keeps the call's address arithmetic inside the
loop in retail. For a parameter the same re-test is redundant and folds away completely. In
`sithPhysics_FindFloor` (`0x0047E4F0`), a loop-top re-test of the function's entry check (`if ( !pThing )`) takes the
function from 1,233 differing bytes to identical. Retail has the entry check.

## 3. Commutative operands are sorted by a key

**Rule.** Before code selection, c2 merge-sorts the operands of every commutative node (`0x409306`) with a comparator
(`0x408d58`) on a 32-bit key. The operand with the higher key comes first: it is the one loaded onto the x87 stack,
copied before an `add`, or used as the SIB base. Equal keys keep source order. The key is `rank << 16 | h16`:

- **rank** is 1 for a leaf and 0 for a constant. An operator's rank combines a Sethi-Ullman depth with its leaf count.
- **h16** depends on the operand:
  - a named variable: the front end's **ordinal for that identifier**, folded to 16 bits;
  - a compiler temporary: its record slot number;
  - a memory operand: the field displacement XOR the base's h16;
  - an operator: its opcode XOR its children's h16.

Keys are computed when the tree is built and survive later rewrites.

Consequences:

- For plain named variables, **declaration order decides operand order**, through the ordinals.
- A field read `p->f` at a non-zero displacement has rank `0x101` and beats every leaf.
- A parenthesis or cast on such a read (`(float)(p->f)`) makes it a load node that is always the loaded operand.

**Witness: `sithPhysics_GetWaterNormalAtPos` (`0x00485770`).** Two x87 operand pairs need opposite orders between three
named objects. No single declaration move fixes both. Retail's loads require the ordinal chain
`startPos < flt_538CD0 < moveDist`: the local `startPos`, a function static and the local `moveDist`. Of the 210 orders
of these declarations, exactly the 35 that satisfy the chain produce retail's bytes (99 -> 0 differing bytes).

**Contrast: Intel C/C++ 4.0** also sorts commutative operands, but its key is the *heap address* of each symbol record
in the compiler's memory. The build therefore needs a model of the 1999 Windows heap to reproduce Intel's output (see
[COMPILERS.md](COMPILERS.md#the-intel-compilers-heap-problem)).

## 4. Which floats stay on the x87 stack

**Rule.** After integer register allocation, a separate pass (`0x45f4b7`) decides which float variables live on the
x87 stack instead of in memory. The candidates are float values in frame slots whose address is never taken. Each gets
a key from its x87 operations, weighted 1 at top level and 4 inside a loop:

| Event | Weight |
| --- | --- |
| a definition | +2 |
| a final `fld` directly before a compare, `fxch` or `fstp` | +1 |
| a reload of a split piece | −1 |
| a spill | −2 |
| each exit edge into a block where the value is not in a register | −1 |

Candidates are taken in key order (ties go to the first assigned). A candidate is refused if:

- another candidate is stored inside its live range on the stack ("last assigned wins");
- its stack nesting would be violated;
- the stack would reach depth 8.

Measured outcomes:

- Every float live across a call is stored.
- In one straight block at most one multi-use float stays: the **last assigned** in source order.
- Use counts do not matter, and `double` behaves like `float`.

**Witness: `sithAIInstinct_HumanCombatMove` (`0x004924D0`).** Retail keeps a value on the x87 stack and later pops it
with `fstp st(0)`: it has no memory home. A text in which the two paths share a block gives it one. The decoded reason is control flow. A path
without the definition joins a block from which a use is reachable, which costs the value a reload and death edges. Its
key drops from +1 to −2, so it is stored. The no-home shape in retail therefore shows that the original's flow graph
had no block shared by the two paths before the last use. That is a fact about the source's control flow, read out of
a register decision.

A related tell distinguishes compiler builds: only the service-pack back ends store a single-use float constant
through the x87 (`fld [K]; fstp [m]`). RTM always uses `mov [m],imm32`
([COMPILERS.md](COMPILERS.md#rtm-sp1-or-sp3-two-tells-in-the-retail-bytes)).

## 5. Tail merging happens once, after register allocation

**Rule.** VC5 SP3's c2 does **no** tail merging before register allocation. Identical code at the end of two branches
is still two copies when frame slots are packed and registers are allocated. It merges only afterwards, in the late
jump optimiser (`0x42d635`), by identity of the machine instructions, registers included. That pass merges in two ways:

- A copy ending `jmp L` merges into the copy that falls into `L`, at any length.
- Two copies that both jump to `L` merge earlier-into-later only when the matched suffix is long enough. The routine
  `0x446590` sums a per-instruction cost and requires the sum to be **greater than 20**, unless nothing falls into the
  earlier copy.

Consequences:

- A block in retail that reloads a value into its home register on several incoming paths was **one block in the
  source**, never a merged duplicate.
- Which of several identical tails survives depends on the order in which the pass meets them.

**Witness: `sithAIInstinct_HumanCombatMove`.** In one spelling of the function, one candidate merge sums 5+4+4+8 = **21** and
is merged. Another sums 5+3+2+6+4 = **20** and is refused, although the match covers that copy's whole head and its
`return 1`.

A caution from the same pass. Because the merge moves a shared boundary up into the argument pushes of a call, retail
code where two paths converge in the middle of a call's pushes can still come from a `goto` to a label at the head of
the shared block. Only compiling that form settles it.

## 6. The number of declarations before a function changes its code

**Rule.** The front end numbers, in source order, every identifier it declares in any scope:

- globals, functions and parameters, including those named in every prototype;
- locals, block `extern`s, typedefs, tags, enum constants and labels;
- one entry per distinct string literal and one per source file opened (a guarded re-include is free);
- two per static local, and one extra per function definition.

The code of some functions depends on that count modulo a period (512 in most cases measured, 2,048 in the longest). Measured unit
costs include: `if` 1, `else` +1, a loop 3, `switch` 4 plus 1 per case body, a prototype = name + parameters (a repeated
one 0). Struct members, macros, redeclarations, expressions, casts and unused tags cost nothing.

**Witness: `sithCogFunctionThing.c`.** Three functions (`IsGhostVisible` at `0x0046CF70`, `SetThingHeadOrientation`
and `GetThingJointPos`) differed from retail by 6, 6 and 1 bytes. All three became exact when filler declarations
shifted the count to any of 8, 9, 22, 24, 25, 38, … (mod 512), and at no count outside such windows. A labelled block
of placeholder declarations, standing for declarations of the original headers that do not survive, makes such a file
match; the rule and its limits are in [DECISIONS.md](DECISIONS.md#2-declaration-count-fits-tier-3).

**How sensitive this is.** The debug build showed that several vector helpers were macros in the original, not
`__inline` functions. Changing them removed their definitions from a widely included header. That shifted the count
of every file including it, and several previously matching files had to be fitted again.

A few functions depend on the absolute count, not a residue: they are exact only above or below a threshold. Their
bounds constrain the size of the original headers.

## 7. Parentheses are code

**Rule.** For a **float** expression, the front end emits one IL operator byte (`0x59`) per parenthesis pair (an `int`
expression's parentheses emit nothing). In c2 such a node:

- becomes a byte-less x87 pseudo-op that counts toward scheduling regions;
- **blocks the algebraic rewrites of `/Og`.**

Two rewrites matter here:

- `a + (int)(x * K)` becomes `a - ftol(x * -K)`, with the constant negated. `a += (int)(x * K)` is not rewritten.
- `x * K + K` becomes `(x - -1) * K`. A parenthesised product, `(x * K) + K`, keeps a multiply followed by a subtract
  of −K (`fmul K; fsub -K`).

`/Od` emits the same code with or without the parentheses, so the unoptimised debug build cannot show them.

**Witnesses.** `jonesConfig_SetDialogTitleAndPosition` (`0x00404AF0`): the debug build's text
`right + (int32_t)((1.0 - s) * 128)` produces retail's pooled constants −128.0 and −96.0 through the first rewrite. A
spelling with the negative constant written out had looked exact only because relocation masking hides constant
values. `SoundDriver_GetIntPan` (`0x004DEE70`) is identical in both builds only as `(int)((pan * 128.0) + 128.0)`.

## 8. Constant flags are threaded only in simple shapes

**Rule.** c2 removes a test of a flag variable whose value is known on each incoming path (jump threading, `0x408bae`).
It does so only if **every** jump to the test's label is preceded directly by `v = <constant>` and the code falling
into the label also sets `v` to a constant. It is all or nothing, and it handles any comparison and any constant, not
only zero. Flat `if`/`else if` chains, `break` out of a loop or `switch`, and an inlined helper's `return K` qualify.
An `if`/`else` nested inside a then-arm does not. So when a decision's early exit leaves from inside the first arm of
an `if`/`else`, no `if`/`else` spelling reproduces retail's code. Only a loop `break`, an inlined helper's `return`
or a `goto` does.

Visual C++ 6.0 differs here: its global optimiser threads every constant-flag join measured, including the shapes
VC5 refuses. The VC6-built INSANE and zlib code needs VC6's rules.

## 9. The host does not matter for Microsoft's compilers

Every decision above is keyed on ordinals, slot numbers or deterministic order within c2's own memory pools, never on
a pointer value, and recompiling under 16 different memory layouts confirms it
([COMPILERS.md](COMPILERS.md#running-1990s-compilers-on-a-modern-host)). Any difference in a Microsoft-compiled file would
therefore never be an artefact of running the compiler under wibo. Intel's compiler is the exception.

## What is not decoded yet

- The block-layout pass: where c2 places a shared failure block, and why a retail block can sit after the last `ret`.
- The per-instruction cost table of the tail-merge test.
- Parts of the x87 register-variable pass (its birth test, a free-spill condition, double-precision weights).
- For the functions that match only through a declaration-count fit, the compiler-side effect is fully sized, but
  the original text behind the count has not been found. The project labels those fits rather than invent text for
  them ([DECISIONS.md](DECISIONS.md#2-declaration-count-fits-tier-3)).
