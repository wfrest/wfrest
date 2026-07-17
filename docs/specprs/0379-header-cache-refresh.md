# Refreshable request header caches

Issue: #379

## Summary

Make `HttpReq::fill_header_map()` replace its derived header snapshot and
invalidate cookie parsing state on every call, so repeated fills reflect the
current underlying `HttpRequest` instead of accumulating history.

## Why

`HttpReq` inherits the protocol message header mutation APIs and publicly
exposes `fill_header_map()`. The current implementation iterates the parser and
appends each field to `headers_`, but never clears fields copied by an earlier
call. If a header is replaced in the parser and the map is filled again,
`header()` still returns the old first cached value.

Cookie parsing adds a second stale layer. `cookies()` lazily builds `cookies_`
once and sets `cookies_parsed_`. A later header fill neither clears that map nor
resets the flag, so new `Cookie` fields are never parsed.

A pre-fix probe replaces `Cookie: session=old` with
`Cookie: session=new`, refills, and prints the public header and cookie views:

```text
session=old old
session=old old
```

The second line should describe the new parser state.

## Goals

- Treat each fill as a complete snapshot of current parser headers.
- Commit the new snapshot only after the cursor scan completes.
- Preserve repeated-field arrival order within one parser snapshot.
- Preserve case-insensitive header-name grouping.
- Invalidate cookies whenever their header source is refreshed.
- Make an unchanged repeated fill idempotent.
- Preserve first-fill behavior in the server request path.
- Keep public signatures and `HttpReq` layout unchanged.

## Non-goals

- Automatically tracking every inherited header mutation before a refill.
- Calling `fill_content_type()` implicitly.
- Invalidating body, form, multipart, or JSON application caches.
- Making concurrent mutation and request access thread-safe.
- Changing cookie parsing, duplicate precedence, or validation.
- Changing the underlying Workflow parser representation.

## Header snapshot contract

`fill_header_map()` scans every field exposed by the current protocol parser
into a new local `HeaderMap`. For a given case-insensitive name, values remain
in parser cursor order.

After the scan, the new map replaces `headers_` in one swap. Values that no
longer exist in the parser disappear, changed values replace their old cached
forms, and unchanged repeated calls produce the same public view without
adding duplicate copies.

The local-build-and-swap structure avoids exposing a partially rebuilt cache
through the object if scan construction is interrupted. WFRest still uses its
existing allocation-failure behavior under `-fno-exceptions`.

## Cookie invalidation contract

After installing a refreshed header snapshot:

- clear `cookies_`;
- set `cookies_parsed_` to false.

The next `cookies()` or `cookie()` call lazily parses all current `Cookie`
fields exactly once under the existing cookie rules. This also handles cookie
access that occurred before any headers were filled: a later fill makes newly
available cookie fields visible.

References to header or cookie strings obtained before a refill are
invalidated by the replacement operation and must not be retained across it.

## Content-type relationship

`fill_header_map()` continues to refresh only the generic and cookie-derived
header caches. `fill_content_type()` remains a separate explicit operation,
as used by `HttpServer::process()` immediately after the header fill.

This change does not silently reinterpret body or form caches when an
application mutates protocol headers after request processing has begun.

## Implementation plan

1. Create a local `HeaderMap` at the beginning of `fill_header_map()`.
2. Populate that map from the protocol header cursor instead of appending to
   the member map.
3. Deinitialize the cursor before committing the result.
4. Swap the completed map into `headers_`.
5. Clear the derived cookie map and reset its parsed flag.
6. Add focused `HttpReq` unit coverage for refresh and idempotence.

## Verification plan

### Before-first-fill cache state

- access `cookies()` on a fresh request and observe an empty cached result;
- add a `Cookie` field to the protocol message;
- fill headers and verify the cookie becomes visible.

### Mutation and refill

- fill ordinary and cookie headers with initial values;
- replace both through inherited protocol APIs;
- refill and verify `header()`, `cookies()`, and `cookie()` expose only the
  replacement values;
- verify removed historical cookie names do not remain cached.

### Idempotence and repeated fields

- include multiple values for one header in the parser;
- refill without mutation;
- verify the first-value public accessor is unchanged;
- use cookie duplicate precedence to prove values were not appended twice and
  continue following one-snapshot arrival order.

### Build and regression

- production C++11 and test C++14 builds with `-Werror` and
  `-fno-exceptions`;
- focused AddressSanitizer and UndefinedBehaviorSanitizer execution;
- complete registered CTest suite;
- cached `git diff --check`.

## Compatibility

The normal server path calls `fill_header_map()` once, so its behavior and
cost remain effectively unchanged apart from building into a local container
before a constant-time swap.

Repeated calls intentionally change from append-only accumulation to refresh
semantics. Code depending on stale first values or ever-growing duplicate
storage relied on an implementation defect. Current parser values, header
case matching, repeated-field ordering, and cookie first-value precedence are
preserved.

No symbol, signature, or object-layout change is introduced.

## Risks

Replacing containers invalidates previously returned references. Such
references were already unstable when `fill_header_map()` appended enough
data to reallocate vectors, and callers that explicitly refresh derived state
must reacquire views afterward. The contract makes this invalidation explicit.

Building a temporary map briefly requires storage for both snapshots during a
refill. Request header sets are bounded by the protocol parser and the old map
is released immediately after the swap.

## Acceptance criteria

- The deterministic mutation probe prints the replacement header and cookie.
- Cookie access before a fill does not permanently cache absence.
- Changed parser headers replace old cached values after a refill.
- Historical cookies are removed and current cookies are reparsed once.
- An unchanged repeated fill is idempotent.
- First-fill server behavior remains unchanged.
- Strict builds, focused sanitizers, full CTest, and diff checks pass.
