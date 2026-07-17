# Safe HTTP/1.x response start lines

Issue: #370

## Summary

Validate the HTTP version, status code, and reason phrase immediately before a
server response is encoded so application-controlled start-line fields cannot
inject response lines or produce invalid HTTP syntax.

## Why

WFRest inherits Workflow's raw setters for all three response start-line
fields. Those setters copy arbitrary NUL-terminated strings without grammar
validation. Workflow's encoder then writes the values directly as:

```text
version SP status-code SP reason-phrase CRLF
```

`HttpServerTask::message_out()` currently supplies missing fields but trusts
fields that are already present. A reason phrase containing CRLF therefore
produces a forged header line on a real connection:

```http
HTTP/1.1 200 OK
X-Injected: yes
Content-Type: text/plain
```

The same injection primitive exists in the version and status-code positions.
The missing-reason fallback also calls `atoi()` on the supplied status string,
which accepts prefixes and provides no exact, bounded syntax check.

## Goals

- Prevent CR, LF, and other forbidden controls in response start lines.
- Emit only supported HTTP/1.x versions.
- Emit exactly three decimal status digits from 100 through 999.
- Preserve safe custom reason phrases and extension status codes.
- Keep the existing default response `HTTP/1.1 200 OK`.
- Convert invalid application state into a deterministic safe response.
- Keep public signatures and class layout unchanged.

## Non-goals

- Removing or hiding Workflow's inherited raw setters.
- Adding HTTP/2 or HTTP/3 serialization.
- Restricting status codes to the IANA registry or to 100–599.
- Transliteration or Unicode normalization of reason phrases.
- Changing response header sanitization, body framing, or request parsing.
- Reporting the normalization through a new callback or error API.

## Version policy

The server transport supports these exact, case-sensitive values:

```text
HTTP/1.0
HTTP/1.1
```

A missing version retains the existing `HTTP/1.1` default. Any present value
outside the allowlist, including a value with whitespace or CR/LF, is replaced
with `HTTP/1.1`.

## Status-code policy

A valid code contains exactly three ASCII digits and has numeric value 100
through 999 inclusive. Parse it with fixed digit arithmetic only after all
three bytes and the terminating NUL have been checked.

This permits extension codes such as 599 or 799 while rejecting examples such
as:

```text
99
099
200x
+200
200 CRLF X-Injected: yes
999999999999999999999999
```

A missing code keeps the existing default of 200. A present but invalid code
is an application-output failure and is replaced with `500 Internal Server
Error`.

No `atoi()`, `strtol()`, locale-sensitive classification, or overflow-prone
accumulation is needed.

## Reason-phrase policy

Use the HTTP/1.1 field-content byte set:

- horizontal tab (`0x09`);
- space (`0x20`);
- visible ASCII (`0x21` through `0x7e`);
- obs-text (`0x80` through `0xff`).

An empty but present phrase remains valid for compatibility. Reject all other
control bytes, especially CR, LF, and DEL.

When the phrase is missing or invalid and the status code is valid, regenerate
the phrase through Workflow's status helper. Registered codes receive their
standard phrase and extension codes receive `Unknown`. Regeneration also
rewrites the same validated numeric code, never the unsafe original text.

## Normalization order

1. Normalize the version independently.
2. Inspect the status-code pointer.
3. If missing, set `200 OK`.
4. If present but invalid, set `500 Internal Server Error`.
5. If valid, preserve it when the phrase is present and valid.
6. Otherwise regenerate the phrase for that parsed code.

This order ensures an unsafe reason phrase cannot survive a status correction
and that a valid custom code/phrase pair remains byte-for-byte unchanged.

## Implementation plan

1. Add internal ASCII-only validators in `HttpServerTask.cc`.
2. Replace the `atoi()` fallback with an exact three-digit parser.
3. Normalize all three fields at the beginning of `message_out()`, before
   Workflow builds its output vectors.
4. Keep the existing header, cookie, framing, and keep-alive work after the
   start-line normalization.
5. Extend the server-task test harness to expose finalization for assertions.
6. Add unit cases for defaults, valid custom values, invalid versions/codes,
   and control-bearing phrases.
7. Add a real-server case proving an injected line no longer reaches the
   client header map.

## Verification plan

### Unit behavior

- preserve `HTTP/1.0 799 Custom Status`;
- preserve an empty custom reason phrase;
- default missing fields to `HTTP/1.1 200 OK`;
- normalize injected/unsupported versions to `HTTP/1.1`;
- normalize short, long, non-digit, and injected codes to 500;
- replace CR/LF or DEL reason phrases for an otherwise valid code;
- preserve valid HTAB and obs-text bytes.

### Socket integration

- serve a response whose raw reason phrase contains CRLF plus a forged header;
- verify the client still receives the intended status code;
- verify its phrase is regenerated safely;
- verify the forged header is absent;
- verify body and normal framing remain intact.

The pre-fix socket probe must show the forged header to demonstrate that the
test closes a wire-visible behavior rather than only an internal invariant.

### Build and regression

- strict C++11 production and C++14 test warning-as-error builds;
- focused AddressSanitizer and UndefinedBehaviorSanitizer runs;
- the complete registered CTest suite, remaining at 38 targets because cases
  extend existing executables;
- cached `git diff --check`.

## Compatibility

Responses produced through `HttpResp::set_status()`, normal body helpers, and
valid inherited setters remain unchanged. Safe custom HTTP/1.0 responses,
extension codes, and custom reason phrases remain supported.

Only values that could not form a valid supported HTTP/1.x start line are
rewritten. Invalid status values now produce an explicit 500 response instead
of zero, a prefix-derived code, an overflow-dependent value, or injected
bytes.

## Risks

Applications that deliberately used an unsupported version string or control
characters in a reason phrase will observe normalization. Such output was not
a valid response and could desynchronize clients or intermediaries.

Status codes 600–999 remain permitted for compatibility with extension use,
although some clients may reject them semantically. This change guarantees
syntax and framing safety, not universal client acceptance of extensions.

Reason phrases are legacy and ignored by many clients, but preserving safe
custom phrases avoids unnecessary behavior changes.

## Acceptance criteria

- No CR, LF, DEL, or forbidden control reaches the encoded start line.
- Only `HTTP/1.0` and `HTTP/1.1` versions are emitted.
- Status codes are exactly three digits in the range 100–999.
- Invalid present codes become `500 Internal Server Error`.
- Missing fields retain `HTTP/1.1 200 OK` defaults.
- Safe custom codes and phrases remain unchanged.
- The real-server forged header disappears.
- Strict builds, focused sanitizers, all 38 tests, and diff checks pass.
