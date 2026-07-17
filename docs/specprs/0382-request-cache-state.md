# Explicit request derived-cache state

Issue: #382

## Summary

Track whether request-body decoding and content-type-specific parsing have
already been attempted independently from whether their results are empty.
Each mutable request cache becomes load-once and remains authoritative after
application mutation.

## Why

`HttpReq` currently treats these conditions as equivalent:

- a cache has never been loaded;
- decoding produced an empty body;
- parsing produced a valid empty object or form;
- parsing rejected malformed input and left an empty result;
- application code intentionally cleared a mutable returned object.

The accessors test `empty()` to decide whether to work again. Empty and invalid
client inputs can therefore trigger repeated O(n) decoding or parsing within
one request. Because the APIs return mutable references, clearing a cache also
causes the next access to resurrect the original request data unexpectedly.

A pre-fix probe feeds complete HTTP requests through the Workflow parser,
loads and clears each public cache, then accesses it again:

```text
body=original
form-size=1
json-key=1
multipart-size=1
```

Every cleared cache was rebuilt solely because its current value was empty.

## Goals

- Decode a request body at most once through `body()`.
- Attempt URL-encoded, multipart, and JSON parsing at most once after the
  corresponding content type matches.
- Cache successful empty results and unsuccessful parse attempts.
- Preserve intentional mutation of returned body/form/JSON references.
- Keep first-access values and parsing rules unchanged.
- Carry cache state naturally through request moves.
- Avoid changing the public `HttpReq` layout, symbols, or signatures.

## Non-goals

- Adding explicit public cache invalidation or reparse APIs.
- Automatically tracking inherited parser or header mutation after access.
- Making multiple derived caches coherent after arbitrary caller mutation.
- Changing gzip, URL decoding, multipart grammar, or JSON grammar.
- Returning immutable views or copies instead of current mutable references.
- Making moved-from `HttpReq` accessors generally usable.

## Private state model

Extend the private, source-file-defined `ReqData` object with four flags:

- body loaded;
- URL-encoded form attempted;
- multipart form attempted;
- JSON attempted.

`HttpReq` continues storing only `std::unique_ptr<ReqData>`, so these fields do
not alter the public class layout. Moving an `HttpReq` transfers the same
`ReqData` allocation and all flags with it.

## Body contract

On the first `body()` call:

1. decode the protocol body/chunk framing;
2. apply the existing gzip handling when selected by cached headers;
3. fall back to decoded wire content under the existing compressor status
   rules;
4. mark the body loaded even when the final string is empty.

Later calls return the current cached string directly. If application code
clears, replaces, or appends to that string, subsequent `body()` calls retain
that state rather than decoding the protocol body again.

## Content-type-specific contract

`form_kv()`, `form()`, and `json()` retain their current type gates. Calling an
accessor before its matching content type is established returns the current
empty cache without marking an attempt. A later `fill_content_type()` can
therefore still enable first parsing.

Once the type matches:

- `form_kv()` parses the current cached body once and records the attempt,
  including an empty map result;
- `form()` parses the current cached body once and records the attempt,
  including invalid/incomplete multipart input;
- `json()` parses the current cached body once, stores a valid result when
  available, and records the attempt even when parsing fails.

After that attempt, the current mutable cache is authoritative. Clearing it
does not implicitly reparse the original body.

## Initialization ordering

The loaded flag is committed after the corresponding local decoding or parse
work has produced its result. WFRest builds with `-fno-exceptions`; allocation
failure retains the process-level behavior already used by these containers.

Derived parsers continue calling `body()`, so they share one decoded input and
cannot repeatedly decompress an empty body.

## Implementation plan

1. Add four default-false booleans to `ReqData` in `HttpMsg.cc`.
2. Replace the body string emptiness guard with the body-loaded flag.
3. Mark the body loaded after existing decode/decompress/fallback logic.
4. Replace each derived container emptiness guard with its attempted flag and
   matching content-type check.
5. Mark each derived attempt after its existing parse/assignment path.
6. Add a request parser harness and focused unit regression matrix.

## Verification plan

### Real parsed body

- parse a non-empty plain request body;
- load it, clear the returned string, and verify later access stays empty;
- verify an initially empty body remains stable across repeated access.

### URL-encoded form

- parse a form with a known field;
- clear the returned map and verify it remains empty;
- cover an empty valid form without repeated-state ambiguity.

### JSON

- parse a non-empty object, clear it, and verify the original key does not
  return;
- access malformed JSON repeatedly and retain one invalid cached result;
- preserve valid empty object/array results.

### Multipart form

- parse a complete field, clear the returned form, and verify it remains
  empty;
- retain an empty result for incomplete or empty multipart bodies.

### Moves

- move a request after loading and mutating a cache;
- verify the destination retains both the cache value and loaded state.

### Build and regression

- production C++11 and test C++14 compilation with `-Werror` and
  `-fno-exceptions`;
- focused AddressSanitizer and UndefinedBehaviorSanitizer execution;
- complete registered CTest suite;
- cached `git diff --check`.

## Compatibility

First calls retain the existing values, decompression status handling, content
type gates, and mutable-reference types. Existing non-empty caches already
behaved as load-once; this specification extends that behavior consistently to
empty caches.

The intentional change is that clearing a returned cache no longer acts as an
undocumented reparse command. Applications needing the original data should
retain it before mutation. Applications needing a fresh parse can use the
public parsing utilities explicitly.

Because `ReqData` is opaque and heap-owned, its private size change does not
alter `sizeof(HttpReq)` or exported function calling conventions.

## Risks

An application may have relied on `clear()` followed by another accessor call
to reconstruct original input. That behavior was coupled to result emptiness,
did not work for non-empty mutations, and caused unbounded repeated parsing of
empty results. Making cache state explicit provides deterministic semantics.

The accessors remain mutable and can still be made mutually inconsistent by
the caller—for example, changing `body()` after `json()` was parsed. Automatic
cross-cache invalidation would make reference mutations surprising in a
different way and is outside this focused fix.

## Acceptance criteria

- The four pre-fix resurrection outputs become empty/zero after clear.
- Empty decoded and parsed results are treated as loaded.
- Invalid JSON and multipart outcomes are attempted only once per request.
- First-access results remain unchanged.
- Caller mutations remain visible on subsequent access.
- Moved-to requests retain loaded state with their `ReqData` allocation.
- Public layout and signatures remain unchanged.
- Strict builds, focused sanitizers, full CTest, and diff checks pass.
