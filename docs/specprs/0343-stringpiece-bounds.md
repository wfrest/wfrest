# Null-safe and saturating StringPiece views

Issue: #343

## Motivation

`StringPiece` is a public non-owning byte view, but its current constructors,
setters, and trimming methods can create invalid pointer/length pairs. Null
C-string inputs reach `strlen`, negative signed lengths become huge unsigned
lengths, and removal larger than the view underflows `size_t`.

Default and cleared instances also store a null pointer. Operations that are
otherwise valid for an empty view then depend on null pointer arithmetic or on
library functions tolerating a null pointer with a zero byte count.

## Goals

1. Keep every empty view usable by iteration, comparison, hashing, copying, and
   string conversion.
2. Normalize null pointer inputs without calling C string functions on null.
3. Prevent signed-length conversion and trimming from increasing or wrapping
   the view size.
4. Preserve the pointer semantics of valid nonempty and caller-positioned empty
   views.
5. Keep the class allocation-free and C++11-compatible.

## Non-goals

- Owning or extending the lifetime of source bytes.
- Validating that a non-null pointer actually spans its claimed length.
- Adding bounds checks to `operator[]`.
- Changing comparison, ordering, or hash algorithms.
- Adding exceptions, assertions, or an error-returning mutation API.

## Empty sentinel

The class provides one private function returning a pointer to a static empty
C string. The following states use that non-null sentinel with length zero:

- default construction;
- `clear()`;
- null C-string construction or assignment;
- null pointer-plus-length construction or assignment, regardless of length;
- a negative `int` length.

Consequently `data()`, `begin()`, and `end()` are non-null and equal in these
states. Empty range iteration performs no dereference, `as_string()` and
`CopyToString()` receive a valid zero-length range, and zero-byte `memcmp`
calls receive valid pointers.

The sentinel is immutable and must never be exposed as writable storage.

## Caller-positioned empty views

A non-null pointer paired with zero length remains unchanged. This includes:

- direct pointer-plus-zero construction;
- `set(non_null, 0)` through either length overload;
- a prefix removal that consumes the complete view.

This preserves useful position information such as a one-past pointer. The
class does not normalize every empty view to the sentinel; it normalizes only
states that otherwise lack a valid pointer or length.

The `std::string` constructor uses `c_str()` so even an empty source supplies a
valid pointer while retaining the source string's lifetime contract.

## Setter rules

`set(const char*)` treats null as `clear()` and otherwise measures the C string.

For `set(const char*, int)`:

- a negative length calls `clear()`;
- zero or positive lengths delegate to the unsigned pointer-plus-length rule.

Pointer-plus-length setters and constructors treat a null pointer as empty and
discard the supplied length. A non-null pointer stores the supplied length
unchanged.

The same rule applies to the `const void*` overload.

## Saturating removal

All removal counts saturate at the bytes currently available.

### Prefix

`remove_prefix(n)` computes `removed = min(n, size())`, advances the pointer by
`removed`, and subtracts `removed` from the length. Removing the exact or an
oversized prefix therefore leaves length zero at the original one-past
position.

### Suffix

`remove_suffix(n)` computes `removed = min(n, size())` and subtracts it. The
begin pointer never changes. Exact and oversized suffix removal leave an empty
view at the original begin position.

### Both sides

`shrink(prefix, suffix)` applies the prefix rule first, then applies the suffix
rule to the remaining view. This ordering is observable only when the combined
request exceeds the original size and prevents either subtraction from
wrapping.

Repeated removals from any empty view are no-ops.

## Element access

`operator[](int)` remains unchecked. Its caller must provide a non-negative
index strictly less than `size()`. This is consistent with the existing API and
separates view-state safety from checked element access.

## Compatibility

Valid in-range construction, setters, trimming, comparison, hashing, and
conversion retain their results and data pointers.

Intentional changes:

- default, cleared, and null-derived `data()` return a non-null empty sentinel;
- null inputs become empty instead of crashing or representing invalid memory;
- negative signed lengths become empty instead of huge;
- oversized trimming empties the available view instead of wrapping length.

Code that used `data() == nullptr` as an out-of-band state must use `empty()` or
track that state separately. `StringPiece` exposes a byte view, not an optional
value.

## Implementation plan

1. Add the private empty-sentinel helper.
2. Normalize constructors, `clear`, and all setter overloads.
3. Implement saturating prefix, suffix, and two-sided removal.
4. Expand `StringPiece_unittest` for null/default states, pointer identity,
   signed lengths, conversions, comparisons, hashes, and oversized/repeated
   removal.

## Validation plan

- rerun the null/negative/over-trim pre-fix sanitizer probe;
- strict C++11 header consumer compile with
  `-Wall -Wextra -Wpedantic -Werror`;
- ASan/UBSan/LSan focused StringPiece and StrUtil tests;
- complete CTest suite;
- `git diff --check` and clean submodule/worktree audit.
