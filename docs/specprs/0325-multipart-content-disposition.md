# Structural multipart Content-Disposition parsing

Issue: #325

## Motivation

Multipart body framing is validated, but each part's `Content-Disposition`
value is still parsed with delimiter splitting and prefix comparisons. That
approach cannot distinguish separators from quoted data and cannot distinguish
the `name` parameter from an unrelated `namex` parameter.

On `test` at `63a4e78`, direct parsing demonstrates that:

- `filename="a=b.txt"` loses its filename;
- `filename="a;b.txt"` is truncated to `"a`;
- `namex="spoofed"` creates a field named `spoofed`;
- `attachment; name="value"` creates a form-data entry.

Uploaded field identity and filename metadata must not depend on substring
spelling or delimiter bytes inside a quoted value.

## Goals

1. Parse `Content-Disposition` with a bounded, quote-aware byte scanner.
2. Require an exact `form-data` disposition type.
3. Match recognized parameter names exactly and case-insensitively.
4. Preserve semicolons and equals signs inside quoted values.
5. Decode quoted-pair escapes exactly once.
6. Reject malformed or ambiguous recognized metadata deterministically.
7. Isolate invalid part metadata from every following part.

## Non-goals

- Decoding RFC 5987 `filename*` values.
- Changing the public `Form` map representation.
- Changing the existing last-part-wins behavior for duplicate form field names.
- Streaming a multipart body across multiple `parse_multipart` calls.
- Sanitizing filenames for use as filesystem paths.

## Accepted syntax

The parser consumes the complete header value using this shape:

```text
OWS disposition-type OWS *( ";" OWS parameter ) OWS
parameter = token OWS "=" OWS ( token / quoted-value ) OWS
```

`OWS` is zero or more ASCII spaces or horizontal tabs. A token contains one or
more RFC HTTP `tchar` bytes: ASCII alphanumeric or one of the following:

```text
! # $ % & ' * + - . ^ _ ` | ~
```

The disposition type must equal `form-data` under ASCII case-insensitive
comparison. Prefixes, suffixes, and other types such as `attachment` are not
accepted for a form part.

Parameter keys are tokens. The recognized keys are exact ASCII
case-insensitive `name` and `filename`. Syntactically valid unknown parameters,
including `filename*`, are parsed and ignored.

### Quoted values

A quoted value begins and ends with `"`. Semicolon and equals bytes inside the
quotes are data, not separators. A backslash followed by a printable byte,
space, or horizontal tab removes the backslash and retains the following byte.
This includes escaped quote and backslash bytes.

Carriage return, line feed, NUL, DEL, another control byte, a trailing
backslash, or a missing closing quote makes the disposition invalid. After the
closing quote, only OWS, a semicolon, or end of input is allowed.

Unquoted values must be non-empty tokens. Values that require separators,
spaces, or non-ASCII bytes must use the quoted form.

## Recognized parameter rules

Exactly one non-empty `name` parameter is required. `filename` is optional and
may be empty when explicitly quoted as `filename=""`.

A second `name` or `filename` parameter invalidates the part even if both
values are identical. Exact matching means that `namex`, `filenamex`, and
`filename*` never populate recognized fields.

Examples:

| Content-Disposition | Result |
| --- | --- |
| `form-data; name="upload"; filename="a=b;c.txt"` | field `upload`, filename `a=b;c.txt` |
| `FORM-DATA; NAME=item; FILENAME="a\"b.txt"` | field `item`, filename `a"b.txt` |
| `form-data; name="item"; filename=""` | field `item`, empty filename |
| `form-data; namex="item"` | discard part |
| `attachment; name="item"` | discard part |
| `form-data; name="a"; name="b"` | discard part |
| `form-data; name="item"; filename="unterminated` | discard part |

## Per-part state and failure behavior

At `part_data_begin`, the parser clears the current header buffers, part data,
recognized values, disposition-seen flag, and disposition-valid flag.

Each completed `Content-Disposition` header is processed once. More than one
such header in a part is ambiguous and invalidates that part. Other headers do
not change disposition state.

At `headers_complete`, the final pending header is processed. At
`part_data_end`, data is inserted into `Form` only when the part has exactly one
valid disposition and a non-empty decoded name. Whether inserted or discarded,
all current-part metadata is cleared before the next part begins.

A malformed part disposition discards only that part. A later valid part is
still returned. Existing whole-body framing rules remain stronger: incomplete
or malformed multipart framing clears the complete form.

## Compatibility

Encoder-generated headers remain valid. Ordinary quoted names and filenames,
unquoted token values, optional unknown parameters, and case variations are
accepted.

Behavior changes only for ambiguous, malformed, or previously misparsed
metadata, plus valid quoted values that contain delimiters or escapes:

- quoted delimiter bytes are preserved;
- false-prefix keys no longer create fields;
- non-form-data dispositions no longer create fields;
- malformed or duplicate recognized parameters no longer partially populate a
  part.

## Implementation plan

1. Add local ASCII helpers for OWS, token bytes, case-insensitive equality, and
   permitted quoted bytes.
2. Implement cursor-based token and value readers that never index beyond the
   header string.
3. Parse the disposition into temporary name and filename values, committing
   them only after the entire value validates.
4. Extend multipart userdata with explicit per-part disposition state.
5. Reset state at part begin and after part end, and gate form insertion on the
   validated state.
6. Replace delimiter splitting in `handle_header` without changing public API.

## Test plan

### Valid values

- quoted filenames containing both `=` and `;`;
- escaped quote and backslash bytes;
- mixed-case disposition type and recognized keys;
- unquoted token values;
- explicit empty filename;
- syntactically valid unknown parameters.

### Rejected part metadata

- `namex`, `filenamex`, and non-form-data dispositions;
- missing and empty names;
- duplicate `name`, `filename`, and `Content-Disposition` headers;
- missing equals, empty unquoted values, trailing semicolon, and junk after a
  quoted value;
- unterminated quote, trailing escape, CR/LF/control bytes, and malformed token
  bytes.

### Isolation and regression

- invalid first part followed by a valid part;
- valid part followed by an invalid part;
- encoder-generated multipart request through `HttpServer`;
- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- focused ASan/UBSan parser tests;
- focused parser and HTTP multipart CTest targets;
- complete CTest suite;
- `git diff --check`.
