# Safe string and path edge handling

Status: Proposed

Issue: [#298](https://github.com/wfrest/wfrest/issues/298)

Integration branch: `test`

## Motivation

Public parsing and path helpers should be total over their documented string
inputs. Three implementation details currently violate that expectation:

- `StrUtil::trim_pairs` calculates and reads the final byte before checking
  whether the input contains any bytes.
- `StrUtil::ltrim` and `rtrim` pass a signed `char` to `std::isspace`.
- `PathUtil::concat_path` reads `front()` and `back()` without first handling
  an empty component.

Each case invokes behavior outside the C++ library contract. The first two are
reachable from request/header parsing, and the path helper is a public API used
with application-controlled values.

## Goals

1. Make the affected utilities safe for every byte string, including empty
   and non-ASCII inputs.
2. Preserve all current behavior for ordinary non-empty ASCII inputs.
3. Define simple, unsurprising identity semantics for empty path components.
4. Lock the behavior down with focused regression tests.

## Non-goals

- Unicode whitespace or grapheme processing.
- Filesystem canonicalization, `.`/`..` resolution, or repeated-slash cleanup.
- Changing `StringPiece` ownership or lifetime rules.
- Broad changes to query, form, or multipart parsing.

## Proposed behavior

### Pair trimming

If the input length is less than two, return the original `StringPiece`
unchanged. Otherwise compare the first and last bytes against the configured
delimiter pairs exactly as today. This length guard prevents both pointer and
size underflow while retaining the zero-copy result.

The `pairs` contract remains a non-null, NUL-terminated sequence of byte pairs;
changing that established contract is not part of this issue.

### Whitespace trimming

Convert the examined byte to `unsigned char` before passing it to
`std::isspace`. This is the required ctype calling convention and preserves
ASCII classification. Bytes above `0x7f` remain ordinary non-whitespace under
the default C locale.

### Path concatenation

Treat an empty component as the identity element:

| Left | Right | Result |
| --- | --- | --- |
| empty | empty | empty |
| empty | non-empty | right unchanged |
| non-empty | empty | left unchanged |

When both components are non-empty, retain the existing four slash-boundary
cases unchanged.

## Validation plan

1. Add `trim_pairs` cases for empty, one-byte, custom identical delimiters,
   and a normal matched pair.
2. Add trim cases whose first or last byte has the high bit set.
3. Add `concat_path` cases for all empty/non-empty combinations.
4. Run the affected GTest binaries under AddressSanitizer and
   UndefinedBehaviorSanitizer.
5. Run the complete functional CTest suite to detect behavior regressions.
6. Run `git diff --check` and a warning-enabled build.

## Compatibility and risk

The change does not alter ABI or function signatures. Empty inputs previously
had no defined result, so defining them cannot break conforming callers.
Non-empty ASCII behavior is covered by the existing tests and must remain
byte-for-byte identical.

## Rollback

The production and test edits can be reverted together. No persisted data,
wire format, build interface, or external dependency changes are involved.
