# Multipart part-header grammar validation

Issue: #358

## Summary

Validate multipart part headers with the HTTP field-name and field-value byte
grammar. Legal token characters must be accepted, field names must be
non-empty, malformed control bytes must reject the body, and field-name state
must remain correct when the low-level parser is fed multiple buffers.

## Why

The multipart state machine currently treats a header field name as a sequence
of ASCII letters and hyphens. That is narrower than the HTTP `field-name`
grammar and rejects valid upload bodies containing headers such as:

```text
Content-MD5: digest
X-Trace_1: value
```

At the same time, the parser accepts a colon before any field-name byte and
does not validate bytes inside field values. As a result, these malformed
headers do not invalidate an otherwise parseable form:

```text
: empty-name
X-Test: value<bare-LF>Injected: yes
X-Test: value<NUL>suffix
```

The current probe reports parsed-form sizes `0, 0, 1, 1` for a legal digit
name, legal underscore name, empty name, and bare-LF value respectively. The
expected results are `1, 1, 0, 0`.

Part headers are a parser trust boundary. Their accepted language should be
defined byte-for-byte instead of depending on a partial character class.

## Goals

- Accept every ASCII `tchar` in a non-empty field name.
- Reject an empty field name and a field line without a colon.
- Accept the HTTP field-value byte domain used by multipart part headers.
- Reject bare line feeds and unsafe control bytes inside field values.
- Treat space and horizontal tab as optional whitespace after the colon.
- Preserve parser correctness when a field name crosses execute buffers.
- Keep boundary detection and form-data metadata semantics unchanged.

## Non-goals

- Supporting obsolete folded header lines.
- Normalizing field names or values.
- Changing callback fragmentation or callback return semantics.
- Adding multipart body, part-count, header-count, or allocation limits.
- Changing `Content-Disposition` parameter parsing.
- Changing multipart boundary validation.
- Exposing a new C++ streaming form API.

## Field-name contract

A field name is one or more ASCII `tchar` bytes:

```text
A-Z a-z 0-9 ! # $ % & ' * + - . ^ _ ` | ~
```

The colon is the delimiter and is not part of the name. A colon encountered
before any name byte is an error. Space, tab, parentheses, slash, backslash,
colon, controls, DEL, and non-ASCII bytes are not field-name bytes.

Examples:

| Header line | Result |
| --- | --- |
| `Content-Disposition: ...` | accept |
| `Content-MD5: digest` | accept |
| `X-Trace_1: value` | accept |
| a punctuation-only name containing every allowed `tchar` | accept |
| `: value` | reject |
| `Bad Name: value` | reject |
| `Bad(Name): value` | reject |
| `Missing-Colon` followed by CRLF | reject |

## Field-value contract

After the colon, leading optional whitespace consists of space or horizontal
tab and is not emitted to the value callback. The remaining field value may
contain:

- horizontal tab (`0x09`);
- space (`0x20`);
- visible ASCII (`0x21` through `0x7e`);
- obs-text (`0x80` through `0xff`).

A carriage return ends the value and must be followed by line feed through the
existing state transition. The following bytes are invalid inside a value:

- NUL and controls `0x01` through `0x08`;
- controls `0x0a` through `0x1f`, including bare LF;
- DEL (`0x7f`).

An empty value, including one containing only leading SP/HTAB before CRLF, is
valid. Trailing SP/HTAB remains part of the callback value, matching the
existing behavior.

Obsolete line folding (`CRLF` followed by whitespace) is not interpreted as a
continuation. The CRLF completes the current header line.

## Header-section termination

A CR encountered in `header-field-start` terminates the part header section
and the existing state machine requires the following LF. A CR encountered
after one or more field-name bytes but before a colon is malformed and must
stop parsing.

This distinction prevents a line such as `Missing-Colon\r\n` from being
mistaken for the blank line that ends headers.

## Incremental execution

`multipart_parser_execute()` may receive a field name across multiple input
buffers. The parser must retain whether at least one valid name byte has been
seen until the colon arrives.

The existing parser index can be reset on entry to `header-field-start`,
incremented for each accepted field-name byte, and checked at the colon. It is
not used for boundary matching while the parser is in header states. Boundary
states continue to reset and own the index for their existing purpose.

Callbacks may still receive multiple fragments when execution is split. This
change does not combine fragments or change their ordering.

## Failure behavior

On the first invalid field-name or field-value byte, the low-level parser stops
at that byte and returns the number of bytes consumed before it, consistent
with existing syntax errors.

`MultiPartForm::parse_multipart()` already requires the parser to consume the
entire body and reach `MP_BODY_END`. Therefore any malformed part header
produces an empty form and never exposes fields accumulated before the error.

## Implementation plan

1. Add local byte predicates for field-name `tchar` and permitted field-value
   bytes in `MultiPartParser.c`.
2. Reset the parser index when a new header field starts.
3. Separate blank-line handling at field start from CR rejection in a partially
   read field name.
4. Increment the persistent name-byte count for each accepted `tchar` and
   require a nonzero count before the colon.
5. Skip both SP and HTAB in `header-value-start`.
6. Validate every non-CR field-value byte before emitting callbacks.
7. Add C++ form-level tests for legal names and malformed values.
8. Add a low-level multi-buffer test that splits a legal field name before its
   digit/token suffix and verifies full consumption and body completion.

## Verification plan

### Valid field names

- conventional alphabetic/hyphen name;
- digit in `Content-MD5`;
- digit and underscore in `X-Trace_1`;
- punctuation members of the complete `tchar` set.

### Invalid field names

- empty name;
- embedded space or tab;
- delimiter punctuation not in `tchar`;
- control, DEL, and non-ASCII byte;
- non-empty line terminated before a colon.

### Field values

- empty value;
- leading SP and HTAB;
- visible ASCII;
- internal/trailing SP and HTAB;
- obs-text;
- NUL, representative controls, bare LF, and DEL rejection.

### Incremental state

- split immediately before a digit/token suffix;
- split immediately before the colon;
- every execute call reports its complete valid chunk consumed;
- callback fragments reconstruct the original field name;
- final body callback is reached.

### Regression

- existing binary part-data and false-boundary behavior;
- existing complete/truncated body handling;
- existing strict `Content-Disposition` tests;
- strict C11 compilation of the parser;
- strict C++14 compilation of the focused test;
- ASan and UBSan focused run;
- full registered CTest suite.

## Compatibility

### Newly accepted

Valid part headers containing digits or the other HTTP token punctuation are
accepted. Their values remain ignored unless an existing consumer recognizes
the field name.

### Newly rejected

Empty field names, missing colons, and field values containing unsafe control
bytes are rejected. These inputs are not valid HTTP field lines and accepting
them was not a stable contract.

### Unchanged

- `Content-Disposition` matching remains case-insensitive.
- Unknown but syntactically valid headers remain ignored.
- Boundary, part-data, duplicate-name, and form replacement behavior remain
  unchanged.
- The public C and C++ signatures do not change.

## Risks

Some senders may have relied on malformed ignored headers being tolerated.
Rejecting such bodies is intentional because a parser should not reinterpret
bare controls or empty names differently from upstream HTTP components.

The state index is shared with boundary matching, so transitions must reset it
at both header-field start and the existing boundary-entry points. Focused
multi-buffer and false-boundary tests guard this state reuse.

## Acceptance criteria

- The reproducer changes from `0, 0, 1, 1` to `1, 1, 0, 0`.
- Every valid `tchar` is accepted in a non-empty part header field name.
- Empty/missing field names and unsafe field-value bytes reject the body.
- A valid field name split across execute calls remains valid.
- Strict C/C++ builds, focused sanitizers, and the full CTest suite pass.
