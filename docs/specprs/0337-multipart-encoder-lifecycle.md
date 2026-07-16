# Owned and complete multipart response encoding

Issue: #337

## Motivation

Multipart response encoding mixes synchronous rendering with asynchronous file
reads. The current callbacks capture a range-for variable by reference, count
invalid files when selecting the final callback, accept short reads, and replace
application-owned SeriesWork context/callback state.

The format layer also emits an invalid empty body, accepts arbitrary boundaries,
and copies quoted metadata without escaping. A safe encoder needs one explicit
ownership and completion model before its formatting can be trusted.

## Goals

1. Own every value and buffer observed by asynchronous callbacks.
2. Select the final task from preflighted valid files only.
3. Attach exactly one complete body or one file-read error.
4. Leave SeriesWork context and callback untouched.
5. Validate boundaries and quoted part metadata.
6. Produce valid empty, parameter-only, file-only, and mixed bodies.
7. Reject short asynchronous reads.

## Non-goals

- Streaming output without buffering all parts.
- Retrying reads or preventing concurrent file mutation.
- Detecting a boundary delimiter collision inside payload data.
- RFC 5987 `filename*` output.
- Changing public encoder signatures or collection types.

## Shared boundary rules

Boundary validation moves to a small header-only multipart utility used by both
`MultiPartForm` and `MultiPartEncoder`. Existing rules remain:

- length 1 through 70;
- ASCII alphanumeric or the allowed multipart punctuation and space;
- no trailing space.

`MultiPartForm::set_boundary` continues to clear parser state on invalid input.
`MultiPartEncoder::set_boundary` instead treats invalid input as a no-op, so the
constructor default or last valid boundary remains usable.

Multipart response Content-Type always quotes the boundary:

```text
multipart/form-data; boundary="boundary-value"
```

The boundary grammar contains neither quote nor backslash, so no additional
escaping is required. Quoting supports valid values containing space or MIME
separator punctuation.

## Quoted metadata

Part `name` and `filename` values use one shared encoder. A value is accepted
when it is non-empty and each byte is HTAB, space through visible ASCII, or
obs-text. CR, LF, NUL, DEL, and other controls reject the metadata.

Double quote and backslash are prefixed with one backslash. Other accepted bytes
are copied unchanged. The structural multipart parser therefore decodes the
value back to its original bytes exactly once.

An invalid scalar field name skips that scalar part. An invalid file field name
or basename filename skips that file during preflight. Payload values and file
bytes remain unrestricted.

## Body rendering

Before every part after the first, append CRLF. A part begins directly with:

```text
--boundary\r\n
```

After the final included part, append CRLF followed by the closing delimiter.
When no parts are included, no leading CRLF is emitted. The empty body is
exactly:

```text
--boundary--\r\n
```

Valid scalar parameters are rendered synchronously before file tasks.

## File preflight

Each registered file is examined before any read task is created:

1. path resolves to a regular file;
2. `FileUtil::size` succeeds;
3. field name encodes successfully;
4. path basename filename encodes successfully;
5. a buffer of `max(size, 1)` bytes is allocated.

An entry failing path, size, or metadata checks is skipped. Allocation failure
marks response setup failed; no file tasks are scheduled and the response is
changed to `StatusFileReadError`.

Only entries passing all five steps become tasks. Their vector order defines
multipart file order, and the last preflighted entry receives `final_task=true`.

## Context ownership

A response context owns the accumulated content, response pointer, boundary,
and failure flag. It is deleted by `HttpServerTask::add_callback`, after no-copy
response output has finished.

Every file task owns a separate context containing:

- a pointer to the response context;
- owned path, encoded field name, and encoded filename strings;
- allocated buffer;
- expected byte count;
- final-task flag.

The file context is installed as task `user_data` before queueing. A server-task
cleanup callback frees its buffer and context. Callback code captures no loop
variable, list element, encoder reference, or boundary reference.

The input `MultiPartEncoder*` is held by a local `unique_ptr` during setup and is
destroyed before the asynchronous tasks run; all required data has already been
copied into owned contexts.

## Asynchronous completion

File tasks run in SeriesWork order, so successful callbacks append parts to the
single response context sequentially.

A task succeeds only when state is `WFT_STATE_SUCCESS`, retval is non-negative,
and retval equals expected size without signed narrowing. Any mismatch sets the
response context failure flag and appends no bytes for that part.

Every task still completes. The explicit final task decides output:

- if any task failed, call `Error(StatusFileReadError)` and do not attach the
  accumulated multipart content;
- otherwise append the closing delimiter and attach the accumulated string
  once with no-copy output.

When preflight yields no file tasks, setup appends the closing delimiter and
attaches the parameter-only or empty content synchronously.

## Series compatibility

The encoder never calls `SeriesWork::set_context` or
`SeriesWork::set_callback`. Applications retain their existing context and
callback. The encoder uses only task queue insertion and additive
`HttpServerTask::add_callback` cleanup hooks.

## Compatibility

Valid existing parameter and file encoders retain order and values. Default
boundary bytes do not change, though the Content-Type parameter becomes quoted.
Invalid files continue to be skipped, but no longer suppress a valid response.

Intentional changes:

- empty bodies become structurally valid;
- invalid boundary setters preserve a valid boundary;
- quotes/backslashes round-trip instead of breaking part headers;
- forbidden metadata is skipped;
- async read failure produces a JSON file-read error rather than partial or
  missing multipart data.

## Implementation plan

1. Add shared boundary and quoted-value helpers.
2. Refactor parser and encoder boundary setters to the shared validator.
3. Introduce response/file context structures and render helpers in HttpMsg.
4. Preflight valid files, allocate buffers, and select an explicit final task.
5. Replace reference-capturing callbacks and SeriesWork state mutation.
6. Enforce exact read completion and deterministic finalization.

## Test plan

- invalid encoder boundaries preserve default/previous valid values;
- empty and parameter-only bodies have exact delimiters;
- quote/backslash field names round-trip;
- control-bearing field names are skipped without part-header injection;
- special valid quoted boundary parses;
- valid files before/after invalid files still finalize;
- quote/backslash basename and field name round-trip;
- empty file part;
- application SeriesWork context/callback remain intact;
- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- ASan/UBSan/LSan multi-file response tests;
- complete CTest suite and `git diff --check`.
