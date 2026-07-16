# Validated response headers and framework-owned framing

Issue: #334

## Motivation

Application response headers are currently stored as arbitrary strings and
copied into Workflow. Workflow's header insertion routine allocates and copies
those bytes without validating their syntax. The Push path separately builds a
raw HTTP header block from the same unvalidated map.

On `test` at `3c19a49`, an HTTP probe demonstrates that:

- CRLF in an `add_header` value creates a second injected header;
- CRLF in `Redirect` location creates a second injected header;
- `Content-Length: 1` truncates a six-byte response at the client;
- `Json()` after `Content-Type: text/plain` emits the invalid value
  `application/json; text/plain`.

The framework must validate generic header syntax at every wfrest-owned output
path and must retain sole control of message framing.

## Goals

1. Accept only valid HTTP field-name token bytes.
2. Reject response field values containing forbidden control bytes.
3. Cover normal responses, raw Push responses, wrapper APIs, and direct map
   writes.
4. Prevent application override of Content-Length and Transfer-Encoding.
5. Give Json responses one valid, idempotent JSON media type.
6. Preserve valid custom headers, obs-text, proxy responses, and SSE behavior.

## Non-goals

- Validating the field-specific grammar of every response header.
- Limiting total response header count or byte size.
- URI syntax validation or open-redirect policy for control-free Location.
- Rewriting already-parsed upstream proxy framing beyond current behavior.
- Preventing an application from explicitly qualifying and invoking a
  `protocol::HttpResponse` base-class mutation method.
- Returning a new error response when an application header is rejected.

## Generic field syntax

The header utility is a C++11 header-only internal component shared by
`HttpMsg.cc`, `HttpServerTask.cc`, and focused tests.

### Field names

A name is valid when it is non-empty and every byte is an RFC `tchar`:

```text
ALPHA / DIGIT / ! # $ % & ' * + - . ^ _ ` | ~
```

This rejects spaces, tabs, colons, separators, controls, DEL, and non-ASCII
bytes in field names.

### Field values

An empty value is valid. Every non-empty byte must be one of:

- horizontal tab (`0x09`);
- space through visible ASCII (`0x20` through `0x7e`);
- obs-text (`0x80` through `0xff`).

All other controls, including NUL, CR, LF, and DEL, invalidate the complete
field. Values are rejected rather than stripped or rewritten, so separate
untrusted pieces can never become a different valid header accidentally.

## Framework-managed fields

Header-name comparison for reserved fields is exact ASCII case-insensitive.
Applications cannot set:

- `Content-Length`;
- `Transfer-Encoding`.

These names are rejected by wfrest add/set wrappers and erased from the public
`headers` map at serialization. Ordinary response code continues to compute
Content-Length from `get_output_body_size()` whenever the retained base response
is not chunked.

Push always erases both application framing names, then emits exactly one:

```text
Transfer-Encoding: chunked
```

It emits no Content-Length. This preserves the manually encoded chunk stream
used by Push.

Already-parsed headers inside a moved Workflow response, such as a proxy
response, are not part of the application map and retain existing behavior.

## Application mutation APIs

`HttpResp::add_header(key, value)` remains a void source-compatible method. It
stores the value only when name and value are valid and the name is not
framework-managed. Rejection is a no-op and leaves any previous valid map entry
unchanged.

`HttpResp` adds string-based `add_header_pair` and `set_header_pair` methods
returning bool. They hide normal unqualified inherited Workflow overloads,
apply the same validation and managed-name rules, return false on rejection,
and otherwise delegate to the base implementation.

The public map remains source-compatible. Because callers can write it
directly, both output paths call a templated sanitizer that erases invalid or
framework-managed entries before using the map.

## Ordinary response serialization

`HttpServerTask::message_out()` sanitizes the map before testing or inserting
defaults. This ordering ensures that:

- an invalid Content-Type does not suppress the default `text/plain`;
- an invalid Date does not suppress the generated Date;
- invalid Connection data cannot be copied by the map;
- application framing cannot suppress the computed Content-Length.

Only the surviving map entries are passed to Workflow. Cookie serialization
continues through its separately validated cookie path.

## Push serialization

`construct_push_header()` sanitizes the same map before concatenating any byte.
It does not depend solely on `add_header`, because the map is public.

After surviving headers are emitted, Push supplies a default Connection when
absent and appends its single chunked Transfer-Encoding field. Invalid names or
values, Content-Length, and application Transfer-Encoding are not present in
the raw header block.

## Redirect behavior

`Redirect(location, status)` writes Location through validated `add_header`.
A control-free location is unchanged. An invalid location is omitted and the
requested redirect status is still set; most importantly, no suffix can become
a second response header.

## Json Content-Type behavior

Both Json overloads use one helper before writing the body.

If the current Content-Type is valid and its media subtype is ASCII
case-insensitive `json` or ends in `+json`, it is preserved, including
parameters. Examples include:

```text
application/json
application/json; charset=utf-8
application/problem+json; charset=utf-8
```

The media type before the first semicolon must contain exactly one slash and
non-empty token type and subtype after trimming space/tab. A non-JSON, malformed,
or control-containing value is replaced with `application/json`. Missing and
empty values are also replaced.

Repeated `Json()` calls are idempotent and never prepend another media type.

## Compatibility

Valid ordinary application headers and valid SSE headers remain unchanged.
Field values may retain HTAB and bytes `0x80` through `0xff`. Case-insensitive
map behavior is unchanged.

Intentional behavior changes affect unsafe or invalid uses:

- injection-bearing fields are omitted;
- invalid direct map entries are erased at serialization;
- application framing entries are ignored;
- Redirect can omit an invalid Location;
- Json replaces non-JSON Content-Type instead of concatenating media types.

The existing low-level base response remains available only through explicit
qualification, which makes bypass an intentional advanced action rather than a
normal wfrest call.

## Implementation plan

1. Add `HttpHeaderUtil.h` with ASCII comparison, token, field-value,
   framework-managed, JSON media type, and map-sanitizer helpers.
2. Move `add_header` out of line and add validated pair wrappers.
3. Sanitize the map at the start of ordinary and Push serialization.
4. Route Redirect and both Json overloads through the new rules.
5. Add direct utility/API tests and a dedicated ordinary response integration
   test.
6. Extend Push integration coverage with invalid direct-map and framing entries.

## Test plan

### Direct validation

- every token punctuation byte and representative valid names;
- empty, whitespace, colon, CR/LF, NUL, DEL, and non-ASCII names;
- empty values, HTAB, visible ASCII, obs-text, and each forbidden control;
- case variants of both framework-managed names;
- valid and invalid JSON media types;
- add/set wrapper return values and rejected-map behavior.

### HTTP integration

- CRLF value injection through `add_header`;
- CRLF name/value injection and managed framing through direct map writes;
- six-byte body with attempted one-byte Content-Length and chunked override;
- Redirect injection omission;
- valid custom header preservation;
- Json replacement, idempotence, and `+json` parameter preservation;
- Push with invalid map fields, attempted framing, one chunked field, and no
  Content-Length.

### Validation

- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- focused ASan/UBSan/LSan utility and integration tests;
- complete CTest suite;
- original live probe after the change;
- `git diff --check`.
