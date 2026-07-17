# Non-throwing request parameter and path accessors

Issue: #373

## Summary

Make typed route-parameter conversion and current-path access safe in
malformed, unparsed, and moved-from request states without changing their
public signatures.

## Why

WFRest compiles production C++ with `-fno-exceptions`, but the inline typed
parameter specializations call `std::stoi`, `std::stoul`, and `std::stod`.
Those functions throw on empty, malformed, and out-of-range values. A client
can supply such text through a normal parameter route, so the convenience API
can terminate the entire server.

`current_path()` separately constructs a `std::string` from
`ParsedURI::path`. That pointer begins null and becomes null again in the
source of a move. Constructing a string from it also terminates in the tested
libstdc++ configuration.

Fresh-process probes reproduce both outcomes:

```text
typed-param exit=134: std::invalid_argument from stoi
current-path exit=134: std::logic_error from string construction
```

## Goals

- Keep typed parameter access non-throwing under `-fno-exceptions`.
- Require a complete, range-valid conversion.
- Preserve the existing zero result for a missing parameter.
- Use the same zero result for invalid typed input.
- Reject negative values for `size_t`.
- Reject non-finite and range-failed floating-point values.
- Preserve valid signed integers, sizes, and finite doubles.
- Return an empty current path when no parsed path exists.
- Preserve public signatures and request object layout.

## Non-goals

- Adding an error-return overload or changing zero/default ambiguity.
- Converting query strings or headers through the typed parameter API.
- Supporting arbitrary integer bases through new arguments.
- Applying application-specific numeric bounds.
- Changing route matching, percent decoding, or URI parsing.
- Making every moved-from `HttpRequest` base-class operation valid.

## Common conversion contract

Each typed specialization first looks up the raw string. Missing keys return
the type's existing zero value.

A present value is valid only when:

- the conversion consumes at least one character;
- the end pointer equals `data() + size()`, not merely the first embedded NUL;
- the conversion reports no range or syntax error;
- the result satisfies the destination-specific constraints below.

This rejects empty strings, trailing junk, trailing whitespace, and embedded
NUL followed by additional bytes. Leading whitespace and the signs accepted
by the corresponding standard conversion remain supported.

The accessors do not throw and do not allocate as part of conversion.

## Integer conversion

Parse `int` with `strtol(..., 10)`. Require the parsed `long` to fall between
`numeric_limits<int>::min()` and `numeric_limits<int>::max()` before casting.

Examples:

```text
"42"       -> 42
" -42"     -> -42
"+17"      -> 17
"12tail"   -> 0
""         -> 0
overflow    -> 0
```

## Size conversion

Parse through `strtoull(..., 10)` and require the result to fit in `size_t`.
After leading C whitespace, reject a minus sign explicitly; unsigned C
conversion otherwise accepts it modulo the result range.

A leading plus remains valid. Zero is valid. Negative, malformed, and
out-of-range values return zero.

## Floating-point conversion

Parse through `strtod()`, require complete consumption, reject `ERANGE`, and
require `std::isfinite()`.

This preserves ordinary decimal/exponent and standard finite conversion forms
while mapping `NaN`, positive/negative infinity, overflow, underflow reported
as range failure, and malformed suffixes to `0.0`.

## Errno behavior

The C conversion functions use `errno` for range reporting. Save the caller's
value before conversion, set it to zero for the operation, capture the result,
and restore the saved value before returning on every converted path.

The convenience accessors therefore do not leak internal range checks into
unrelated application error handling.

## Current path behavior

`current_path()` remains a by-value `std::string` accessor. If
`parsed_uri_.path` is non-null, copy and return it exactly as before. If it is
null, return an empty string.

This covers default construction, a failed/unperformed URI parse, and the
source object after `HttpReq` move construction or assignment.

## Implementation plan

1. Add the required C++11 conversion, range, errno, and finite-value headers
   to `HttpMsg.h`.
2. Add inline internal conversion helpers beside the existing specializations.
3. Replace `stoi`, `stoul`, and `stod` calls with the helpers.
4. Keep lookup and zero-default semantics at the public specialization layer.
5. Null-check `ParsedURI::path` in `current_path()`.
6. Add unit cases to the existing `HttpMsg_unittest` target.
7. Add a real-server parameter case proving malformed client input no longer
   terminates request handling.

## Verification plan

### Unit parsing matrix

- missing and empty values;
- valid positive, negative, signed, exponent, zero, and destination-boundary
  values;
- prefix/suffix junk and trailing whitespace;
- signed and unsigned overflow;
- negative size input;
- embedded NUL with a numeric prefix;
- NaN, infinity, floating overflow, and floating underflow;
- caller `errno` preservation.

### Path lifecycle

- default request returns an empty path;
- parsed request returns its path;
- move destination retains the path;
- move source returns an empty path.

### Server integration

- register a numeric parameter route;
- send a non-numeric segment;
- call all three typed accessors in the handler;
- verify a successful response containing zero defaults;
- send a valid segment and verify normal conversion remains intact.

### Build and regression

- strict C++11 production and C++14 test compilation with `-Werror` and
  `-fno-exceptions`;
- focused AddressSanitizer and UndefinedBehaviorSanitizer execution;
- full registered CTest suite, remaining at 38 targets;
- cached `git diff --check`.

## Compatibility

Signatures, inline availability, and missing-key defaults remain unchanged.
Valid values continue to convert to the same destination types.

The intentional semantic tightening is complete consumption: values such as
`12tail`, which the prior `std::stoi` call silently prefix-parsed because it
did not request the consumed position, now return zero. Negative `size_t`
text and non-finite doubles likewise become zero instead of surprising large
or special values.

Applications that need to distinguish invalid text from a valid numeric zero
can continue reading the raw string through `param(key)` before conversion.

## Risks

C floating conversion follows the process locale, as the previous `std::stod`
implementation did. This change does not introduce a locale-independent
decimal grammar.

Returning zero keeps the established missing-value convention but cannot
express why conversion failed. Adding an optional/result API can be considered
separately without making the current convenience functions unsafe.

## Acceptance criteria

- No typed route parameter causes an exception or process termination.
- Invalid, incomplete, negative-size, out-of-range, and non-finite values
  return zero.
- Valid boundary values convert correctly.
- Conversion restores caller `errno`.
- Default and moved-from `current_path()` return an empty string.
- Parsed and moved-to paths remain unchanged.
- Malformed parameter input succeeds through a real server request.
- Strict builds, focused sanitizers, all 38 tests, and diff checks pass.
