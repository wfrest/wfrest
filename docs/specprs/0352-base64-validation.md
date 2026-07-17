# Complete and canonical Base64 validation

Issue: #352

## Summary

Add checked Base64 encode/decode overloads, make decoding validate the complete
input, and route the legacy string-returning APIs through the checked logic.
Malformed padding, suffixes, characters, lengths, and unused bits fail as one
transaction instead of returning a successfully decoded prefix.

## Why this change is needed

The current decoder stops scanning when it sees `=` or a byte that
`isalnum()` does not accept. It then decodes any pending prefix without
checking the remaining input. Padding is therefore treated as a terminator
rather than grammar, and invalid bytes after a complete quartet disappear.

Examples from the current implementation:

| Input | Current output | Problem |
| --- | --- | --- |
| `TWFu` | `Man` | valid |
| `TWFu$admin` | `Man` | invalid suffix ignored |
| `TQ==` | `M` | valid |
| `TQ=evil` | `M` | padding and suffix ignored |
| `TQ===` | `M` | excess padding ignored |
| `A` | empty | impossible final group accepted as empty |
| `AB==` | byte `0x00` | non-zero unused bits accepted |

This is an ambiguity risk for credentials, signatures, identifiers, and cache
keys. Separately, `encode(nullptr, 1)` dereferences a null pointer.

## Goals

- Expose success/failure separately from decoded bytes.
- Validate every encoded input byte.
- Validate standard padding as grammar, not a stopping condition.
- Require canonical unused bits in the final quantum.
- Continue accepting canonical unpadded two- and three-character tails.
- Keep valid standard Base64 bytes and round trips unchanged.
- Make encode/decode output transactions safe for aliases.
- Validate null pointers and size calculations.
- Keep the legacy source API available.

## Non-goals

- MIME whitespace folding.
- URL-safe Base64 (`-` and `_`).
- Automatically removing surrounding whitespace.
- Constant-time decoding or cryptographic authentication.
- Adding streaming encoder/decoder state objects.
- Changing the standard `+/` alphabet or encoder padding policy.

## Public API

The existing APIs remain:

```cpp
static std::string encode(const unsigned char *data, unsigned int len);
static std::string decode(const std::string& encoded);
```

Add checked overloads:

```cpp
static bool encode(const unsigned char *data,
                   size_t len,
                   std::string *output);

static bool decode(const std::string& encoded,
                   std::string *output);
```

The checked encoder's `size_t` length is not constrained by the legacy
`unsigned int` signature.

### Output transaction

For both checked overloads:

- `output == nullptr` returns `false` and dereferences nothing;
- valid input is encoded/decoded into independent staging storage;
- on success, staging replaces `*output` exactly once;
- on failure, a valid `output` is cleared;
- input may refer to the same string object or backing storage as output,
  provided the input range is valid when the call begins.

### Legacy wrappers

- legacy `encode()` delegates to checked encode and returns its output;
- legacy `decode()` delegates to checked decode and returns its output;
- malformed input returns an empty string, never a decoded prefix;
- callers that must distinguish malformed input from valid empty input use
  the checked boolean overload.

## Encoding contract

### Input pointers

| Input | Result |
| --- | --- |
| `data == nullptr`, `len == 0` | success, empty output |
| `data != nullptr`, `len == 0` | success, empty output |
| `data == nullptr`, `len > 0` | failure, output cleared |
| valid binary range | success |

Embedded NUL and all byte values are data.

### Output grammar

- Use the standard alphabet `A-Z a-z 0-9 + /`.
- Emit four characters for each three input bytes.
- Emit exactly two `=` bytes for a final one-byte group.
- Emit exactly one `=` byte for a final two-byte group.
- Emit no whitespace or line wrapping.

### Size validation

Compute the number of four-character groups without overflowing. Before
reserve or multiplication, require the group count to fit within
`output.max_size() / 4`. A size failure returns `false` and clears output.

## Decoding grammar

### Alphabet

Only these ASCII bytes have sextet values:

```text
A-Z -> 0..25
a-z -> 26..51
0-9 -> 52..61
+   -> 62
/   -> 63
```

Do not use locale-sensitive `isalnum()`. Bytes above ASCII, whitespace, `-`,
`_`, punctuation, control bytes, and embedded NUL are invalid.

### Padding

- Padding may appear only as a contiguous suffix.
- The suffix contains at most two `=` bytes.
- A padded encoded string has total length divisible by four.
- One `=` requires exactly three alphabet bytes in the final quartet.
- Two `=` require exactly two alphabet bytes in the final quartet.
- No alphabet or other byte may follow the first `=`.

### Unpadded input

Canonical unpadded Base64 remains accepted:

- length modulo four `0`: complete quartets;
- length modulo four `2`: one decoded byte;
- length modulo four `3`: two decoded bytes;
- length modulo four `1`: invalid.

Empty input succeeds with empty output.

### Canonical unused bits

The final sextets contain unused low bits that must be zero:

- for a two-character tail or `xx==`, the second sextet's low four bits are
  zero;
- for a three-character tail or `xxx=`, the third sextet's low two bits are
  zero.

This rejects alternate encodings such as `AB==` for byte zero while accepting
the canonical `AA==`.

## Decode algorithm

1. Count a trailing `=` suffix and reject more than two.
2. Require all earlier bytes to be alphabet bytes; reject embedded padding.
3. Validate padded/unpadded length shape.
4. Validate the unused bits of the last partial quantum.
5. Validate decoded-size arithmetic before reserving.
6. Decode complete quartets into three bytes each.
7. Decode a final two- or three-character group into one or two bytes.
8. Assign staged output only after all validation and decoding succeeds.

No `std::string::find()` result is narrowed into a byte value.

## Compatibility table

| Category | Required behavior |
| --- | --- |
| Encoder-produced padded Base64 | unchanged success |
| Canonical unpadded `TQ` / `TWE` | success |
| Empty encoded input | success, empty |
| Invalid suffix after quartet | failure as a whole |
| Whitespace/newline | failure |
| URL-safe alphabet | failure |
| Excess/middle padding | failure |
| Non-canonical unused bits | failure |
| Legacy invalid decode | empty result, no prefix |

## Implementation plan

1. Add `<cstddef>` and the two checked overload declarations to the public
   header.
2. Replace locale-sensitive classification with an internal ASCII sextet
   function returning `-1` for invalid bytes.
3. Implement checked encode with staging, null/size validation, reserve, and
   direct three-byte/final-group transforms.
4. Implement checked decode according to the complete grammar above.
5. Delegate both legacy wrappers to the checked overloads.
6. Replace the two happy-path-only tests with known vectors, binary, alias,
   malformed, canonical-bit, and null-boundary tables.

## Verification plan

### Known vectors

- `"" <-> ""`
- `f <-> Zg==`
- `fo <-> Zm8=`
- `foo <-> Zm9v`
- `foob <-> Zm9vYg==`
- `fooba <-> Zm9vYmE=`
- `foobar <-> Zm9vYmFy`
- padded and canonical unpadded forms decode identically.

### Binary and size boundaries

- All byte values round trip.
- Payload sizes around three-byte group boundaries round trip.
- Checked encode accepts `size_t` lengths and guards output-size arithmetic.

### Invalid table

- invalid suffixes: `TWFu$admin`, `TWFu\n`;
- invalid padding: `TQ=evil`, `TQ===`, `=TQ=`, `T=Q=`;
- invalid lengths: `A`, `AAAAA`;
- invalid alphabet: whitespace, URL-safe bytes, high bytes, embedded NUL;
- non-canonical bits: `AB==`, `AAB=`, and unpadded equivalents;
- every failure clears a stale output.

### Aliases and nulls

- checked decode supports `decode(value, &value)`;
- checked encode supports raw input backed by its output string;
- null output fails without access;
- null data with non-zero length fails and clears;
- null data with zero length succeeds empty.

### Tooling and regression

- Re-run the original malformed/null probe under ASan/UBSan.
- Compile production and test sources with strict warning-as-error flags.
- Run Base64 tests under ASan/UBSan.
- Run the complete registered CTest suite.

## Risks and mitigations

### Tightening malformed behavior

Some callers may have depended on prefix decoding. That behavior makes
malformed and clean identifiers equivalent. Keep valid padded and canonical
unpadded forms compatible, document the invalid-only change, and provide a
boolean status API for explicit migration.

### Input/output identity

Clearing output before parsing would reproduce the compression alias bug.
Always stage first and clear only after an invalid input has been fully
classified or a size check fails.

## Acceptance criteria

- The fixed malformed examples all fail through the boolean API.
- Legacy decode returns no partial prefix for any invalid input.
- Null encoding input does not trigger sanitizer findings.
- Canonical padded and unpadded vectors decode correctly.
- Non-zero unused bits are rejected.
- Aliased checked operations preserve their original input until commit.
- Strict builds, focused sanitizer tests, and the full suite pass.
