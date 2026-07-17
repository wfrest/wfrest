# Owned temporary query fallbacks

Issue: #376

## Summary

Make `HttpReq::default_query()` safe when its fallback is a temporary string,
while retaining the existing reference-returning API for caller-owned lvalue
fallbacks.

## Why

The current function accepts `const std::string& default_val` and returns
`const std::string&`. When the requested key is absent, it returns
`default_val` directly. A string literal or temporary `std::string` binds to
that parameter only until the end of the calling full expression. Keeping the
returned reference and using it in a later statement is therefore undefined
behavior.

The unsafe spelling is natural and does not advertise a lifetime requirement:

```cpp
const std::string& page = request.default_query("page", "1");
use(page); // page already dangles when the query was absent
```

A focused pre-fix AddressSanitizer probe reports `stack-use-after-scope` when
reading the returned string on the next statement.

Changing the existing function to return by value is not an appropriate
compatibility fix. Ordinary C++ symbol names do not encode the return type, so
an application compiled against a by-value declaration could link to an older
reference-returning definition with an incompatible calling convention.

## Goals

- Give temporary and string-literal fallbacks an owned result.
- Preserve the existing lvalue overload, return type, and symbol.
- Keep query-present and query-absent selection semantics unchanged.
- Preserve zero-copy fallback access for caller-owned lvalues.
- Make overload selection visible in compile-time regression tests.
- Avoid changing `HttpReq` object layout or request parsing.

## Non-goals

- Making references to caller-owned lvalue fallbacks outlive those lvalues.
- Changing the lifetime of references returned by `query()`.
- Adding typed query conversion or optional/result wrappers.
- Changing duplicate-query parsing or percent decoding.
- Removing the existing overload in this compatibility release.

## Public API contract

Retain the existing overload exactly:

```cpp
const std::string& default_query(
    const std::string& key,
    const std::string& default_val) const;
```

Add an overload for temporary fallback ownership:

```cpp
std::string default_query(
    const std::string& key,
    std::string&& default_val) const;
```

An lvalue `std::string` binds to the first overload. A temporary
`std::string`, including the conversion created for a string literal, binds to
the second overload because an rvalue reference is the better binding.

## Result behavior

For the retained lvalue overload:

- a present key returns a reference to the stored query value;
- an absent key returns the same caller-owned fallback reference.

For the new rvalue overload:

- a present key returns an owned copy of the stored query value;
- an absent key moves the temporary fallback into the returned value.

The by-value result may safely bind to a `const std::string&` at the call site;
normal temporary lifetime extension then keeps it alive for the reference
variable's scope.

## Implementation plan

1. Declare the rvalue overload beside the current function in `HttpMsg.h`.
2. Implement it in `HttpMsg.cc` without altering the current definition.
3. Use one map lookup per overload rather than `count()` followed by `at()`.
4. Copy the stored value on a hit and move `default_val` on a miss in the
   owning overload.
5. Add compile-time assertions for lvalue, temporary, and literal overload
   result types.
6. Add runtime tests for hit/miss behavior and lvalue reference identity.

## Verification plan

### Compile-time API matrix

- lvalue fallback result is `const std::string&`;
- temporary-string fallback result is `std::string`;
- string-literal fallback result is `std::string`.

### Runtime matrix

- lvalue miss aliases the supplied fallback;
- lvalue hit aliases the stored query value, not the fallback;
- temporary miss owns the fallback contents;
- temporary hit owns the stored query contents;
- a by-value result bound to `const std::string&` remains valid.

### Lifetime regression

Rebuild the independent pre-fix probe with AddressSanitizer and
`-fsanitize-address-use-after-scope`. The old implementation must report a
stack lifetime violation; the owning overload must complete without a
sanitizer report.

### Build and regression

- production C++11 and test C++14 compilation with `-Werror` and
  `-fno-exceptions`;
- focused AddressSanitizer and UndefinedBehaviorSanitizer execution;
- the complete registered CTest suite;
- cached `git diff --check`.

## Compatibility

Existing lvalue calls continue selecting the original ABI and preserve
reference identity. Existing calls that immediately copy a temporary fallback
still produce the same text, now through the owning overload. Calls that store
a reference to a temporary fallback change from undefined behavior to a safe
lifetime-extended owned result.

The new overload can change overload resolution only for rvalue fallback
arguments, which are precisely the arguments the old reference-returning
contract could not safely expose past the full expression.

## Risks

A query hit through the new overload requires a string copy, whereas the old
unsafe call returned a reference. Returning a reference on a hit and a value
on a miss cannot be expressed with one static return type. Correct lifetime
therefore takes priority for this temporary-only overload.

The lvalue overload still requires the caller not to retain the result beyond
the request or fallback object it aliases. That pre-existing and valid
reference contract remains documented by its return type.

## Acceptance criteria

- Temporary and literal fallbacks select a by-value overload.
- Missing temporary fallbacks remain valid after the call expression.
- Present queries are returned correctly for both overloads.
- Lvalue fallback calls keep reference identity and the old symbol.
- The pre-fix sanitizer failure no longer reproduces.
- Strict builds, focused sanitizers, full CTest, and diff checks pass.
