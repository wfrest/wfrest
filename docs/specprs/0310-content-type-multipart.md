# Exact Content-Type and complete multipart parsing

Issue: #310

## Motivation

`Content-Type` selects the request body parser exposed to application handlers.
The current selection is a case-sensitive prefix match, while multipart boundary
extraction is a case-sensitive substring search that retains the rest of the
header value. These choices create opposite failures:

- lookalike types such as `application/jsonp` are classified as JSON;
- valid case variants such as `Multipart/Form-Data` are not classified;
- valid multipart requests fail when `boundary` has optional whitespace, is
  quoted, is capitalized, appears after another parameter, or is followed by
  another parameter;
- a malformed delimiter suffix can commit a field before the multipart parser
  discovers that the body did not terminate correctly.

The low-level parser initializer also assumes non-null inputs and successful,
non-overflowing allocation.

## Goals

1. Classify a known media type by exact, ASCII-case-insensitive token equality.
2. Parse multipart parameters structurally instead of searching raw text.
3. Reject missing or invalid multipart boundaries deterministically.
4. Expose form fields only after a complete, valid closing delimiter.
5. Make parser initialization fail safely without changing the C API shape.
6. Preserve successful behavior for existing valid request and encoder output.

## Non-goals

- Rewriting `Content-Disposition` field and filename parsing.
- Adding streaming or incremental multipart support to `MultiPartForm`.
- Adding a collection type for duplicate multipart field names.
- Interpreting arbitrary media-type parameters outside boundary extraction.

## Media-type classification

`ContentType::to_enum` treats the input as:

```text
optional-whitespace media-type optional-whitespace
    *( ";" parameter-text )
```

Only the portion before the first semicolon participates in classification.
Leading and trailing HTTP optional whitespace (`SP` and `HTAB`) around that
portion is ignored. The remaining media type must equal a known type in full,
using ASCII case-insensitive comparison.

Examples:

| Input | Result |
| --- | --- |
| `application/json` | `APPLICATION_JSON` |
| `Application/JSON; charset=utf-8` | `APPLICATION_JSON` |
| ` application/json \t` | `APPLICATION_JSON` |
| `application/jsonp` | `CONTENT_TYPE_UNDEFINED` |
| `multipart/form-datax; boundary=x` | `CONTENT_TYPE_UNDEFINED` |
| empty or whitespace only | `CONTENT_TYPE_NONE` |

This changes only false-positive prefixes and case/whitespace variants. Suffix
lookup remains exact and case-sensitive because suffixes are registry keys, not
HTTP media-type tokens.

## Multipart boundary parameter

After multipart classification, parameters are scanned from left to right at
semicolon boundaries. Parsing observes quoted strings, so a semicolon inside a
quoted value is not treated as a parameter separator. Parameter names use exact
ASCII case-insensitive equality. Optional whitespace is allowed around names,
the equals sign, and values.

For the `boundary` parameter:

- exactly one occurrence is required;
- an unquoted value ends at the next semicolon and may not contain whitespace;
- a quoted value ends at its matching quote and supports backslash-escaped
  characters;
- characters following a quoted value must be optional whitespace followed by
  a semicolon or end of input;
- CR and LF are invalid in names and values;
- the decoded boundary length must be 1 through 70 bytes;
- every byte must be an RFC multipart boundary character: ASCII alphanumeric,
  apostrophe, parentheses, plus, underscore, comma, hyphen, dot, slash, colon,
  equals, question mark, or an internal space;
- the final byte may not be a space.

Unrelated well-formed parameters are ignored. A malformed parameter section,
duplicate boundary, missing boundary, empty value, invalid byte, or overlong
value leaves the request classified as multipart but installs no boundary.
Calling `form()` then returns an empty form.

Examples accepted:

```text
multipart/form-data; boundary=abc
Multipart/Form-Data; charset=utf-8; Boundary = "abc"; version=1
multipart/form-data; boundary="a:b?c"
```

Examples rejected for multipart parsing:

```text
multipart/form-data
multipart/form-data; boundary=
multipart/form-data; boundary=a; boundary=b
multipart/form-data; boundary="unterminated
multipart/form-data; boundary="abc"junk
```

## Complete-body contract

`MultiPartForm::parse_multipart` returns an empty form unless all conditions
below hold:

1. a non-empty validated boundary was installed;
2. low-level parser initialization succeeded;
3. `multipart_parser_execute` consumed the complete input body;
4. the `body_end` callback was reached through a closing `--boundary--`.

The parser may build a temporary form while scanning, but partial data is not
observable when any condition fails. This is intentionally all-or-nothing for
the existing non-streaming API.

Trailing CRLF or epilogue bytes remain accepted after a valid closing delimiter,
matching existing parser behavior. A truncated body, a regular inter-part
delimiter at end of input, or a boundary followed by an invalid suffix returns
an empty form.

## Low-level parser initialization

`multipart_parser_init` keeps its pointer-returning C signature. It returns
`NULL` when the boundary or settings pointer is null, the boundary is empty,
the allocation-size calculation would overflow, or allocation fails. The data
pointer is initialized to `NULL`. Null-safe free and guarded accessors avoid a
secondary crash on the failure path.

Public request parsing validates the 70-byte protocol limit before calling the
initializer; the overflow guard protects direct low-level callers as well.

## Compatibility

Valid lower-case media types, encoder-generated multipart bodies, unquoted
boundaries, and quoted boundaries continue to parse. Newly accepted inputs are
case variants and structurally valid parameter layouts.

Behavior changes intentionally for invalid inputs:

- known-type prefixes are no longer accepted;
- malformed or ambiguous boundary parameters no longer reach the body parser;
- incomplete multipart bodies no longer expose fields parsed before failure.

No public C++ method signature changes.

## Test plan

### ContentType unit tests

- exact known type and parameterized known type;
- ASCII case variants and surrounding SP/HTAB;
- empty and whitespace-only values;
- false-positive prefixes and unknown types.

### MultiPartForm unit tests

- valid single and multiple fields;
- empty or missing installed boundary;
- truncated header/data/body;
- invalid delimiter suffix after matching boundary bytes;
- ordinary inter-part and final delimiters;
- binary field data and boundary-like data that is not a delimiter;
- null and invalid low-level initializer arguments.

### HTTP integration tests

- mixed-case media type and `Boundary` parameter;
- optional whitespace, quoted boundary, earlier and later unrelated parameters;
- missing, duplicate, malformed, invalid-character, and overlong boundaries;
- malformed closing delimiter does not expose a form field;
- existing encoder-generated requests remain green.

### Validation

- focused tests under AddressSanitizer and UndefinedBehaviorSanitizer;
- focused CTest runs for `HttpDef`, `HttpContent`, and the HTTP multipart test;
- complete CTest suite;
- C++11/C90 warning builds for touched parser code;
- `git diff --check`.
