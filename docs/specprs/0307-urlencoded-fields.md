# Lossless URL-encoded field parsing

Status: Proposed

Issue: [#307](https://github.com/wfrest/wfrest/issues/307)

Integration branch: `test`

## Motivation

Query strings and URL-encoded form bodies are structurally the same sequence
of `&`-separated fields. wfrest currently implements the same vector-based
parser twice. Both split a field on every equals sign and retain only the
second token, so common opaque values such as signed tokens and Base64 padding
are silently truncated.

Neither parser decodes percent escapes or plus-as-space. The available decoder
also accepts malformed hex through `strtol`, turning `%ZZ` into an embedded NUL
instead of preserving the input.

## Goals

1. Preserve every byte after the first structural `=` as the field value.
2. Apply URL decoding after structural `&`/`=` boundaries are identified.
3. Make malformed percent escapes lossless while retaining valid binary
   decoding, including `%00`.
4. Use one parser for query strings and URL-encoded form bodies.
5. Eliminate intermediate split vectors and lock behavior down with direct
   tests registered in CTest.

## Non-goals

- Returning multiple values for duplicate keys; the public result is a map.
- Unicode normalization or charset conversion.
- Path encoding rules or changing `url_encode` treatment of `/`.
- Multipart form parsing.

## Decoder behavior

`CodeUtil::url_decode` scans once and reserves `value.size()`, because decoding
never produces more bytes than the input.

- `+` appends a single space.
- `%` followed by exactly two hexadecimal digits appends the decoded byte and
  consumes both digits.
- A `%` without two following bytes, or with either non-hex digit, is appended
  literally; following bytes are processed normally.
- All other bytes are copied unchanged.

Hex conversion uses an explicit nibble helper rather than `strtol`, avoiding a
temporary substring, locale concerns, and acceptance of malformed input.

## Field parser

`UriUtil::split_query` uses two cursors over the input:

1. Find the next `&` or end to define one raw field.
2. Skip empty raw fields.
3. Find only the first `=` within that field.
4. If no `=` exists, the raw value is empty; otherwise the raw value is every
   byte after that first delimiter.
5. Decode key and value independently.
6. Skip a decoded-empty key.
7. Insert with `map.emplace`, preserving the first decoded duplicate.

Structural splitting precedes decoding, so `%26` remains inside a field and
`%3D` remains inside its key/value component until decoded.

`Urlencode::parse_post_kv` delegates directly to this function. This makes
query and form semantics identical by construction.

## Examples

| Input field | Key | Value |
| --- | --- | --- |
| `token=a=b=c` | `token` | `a=b=c` |
| `message=hello+world` | `message` | `hello world` |
| `expr=a%26b%3Dc` | `expr` | `a&b=c` |
| `flag` | `flag` | empty |
| `bad=%ZZ` | `bad` | `%ZZ` |
| `%61=first&a=second` | `a` | `first` |

## Test plan

1. Add `UriUtil_unittest` for structure, decoding, duplicates, empty fields,
   malformed escapes, and embedded NUL.
2. Populate and register `HttpContent_unittest`; compare form results against
   the query parser for the same payload matrix.
3. Extend `CodeUtil` coverage with valid upper/lower hex, plus, malformed,
   truncated, and binary cases.
4. Run the focused utilities with AddressSanitizer and
   UndefinedBehaviorSanitizer.
5. Run the complete CTest suite and warning-enabled C++11 build.

## Compatibility and risk

Correctly encoded keys and values now appear decoded to handlers, which is the
intended semantic change. Callers that manually decoded `req->query()` or
`form_kv()` results must avoid decoding twice. Unescaped ordinary ASCII fields
retain existing values, except that embedded equals signs are no longer lost.

Malformed escapes remain literal rather than producing NUL, which is safer and
lossless. First-value-wins duplicate behavior remains, but comparison occurs
after decoding so aliases are deterministic.

## Rollback

Revert the decoder, shared parser delegation, test registration, and tests as
one unit. No public signature, wire response, or persisted representation is
introduced.
