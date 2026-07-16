# Deterministic HTTP message move ownership

Issue: #319

## Motivation

`HttpReq` and `HttpResp` extend Workflow HTTP messages with derived state, but
their construction and move operations do not maintain that state's lifetime:

- the `HttpReq(HttpRequest&&)` path leaves its owning `ReqData *` and content
  type indeterminate;
- request move assignment overwrites the destination pointer without deleting
  it;
- request self-move nulls the pointer through its source alias;
- response `user_data` is indeterminate after ordinary construction;
- response self-move clears `user_data` and self-moves containers.

Workflow's base `HttpMessage` move assignment already guards self-assignment and
releases its parser and output blocks. wfrest must provide the same invariants
for every derived member.

## Goals

1. Give request cache storage explicit single ownership.
2. Initialize every public constructor completely.
3. Make move assignment release the destination before taking the source.
4. Make self-move a no-op for request and response objects.
5. Transfer all base and derived observable state exactly once.
6. Preserve the existing public class and method surface.

## Non-goals

- Owning or deleting `HttpResp::user_data`; it remains an opaque, non-owning
  pointer supplied by applications.
- Changing Workflow's base HTTP move implementation.
- Guaranteeing that request data access is meaningful on a moved-from object
  before that object receives a fresh value.
- Making these noncopyable messages copyable.

## Request cache ownership

The private `ReqData *` becomes `std::unique_ptr<ReqData>`. `ReqData` remains
forward-declared in the public header; constructors, destructor, and move
operations that need the complete type are defined in `HttpMsg.cc`.

All non-move construction paths allocate one cache object:

| Constructor | Base state | Derived initialization |
| --- | --- | --- |
| `HttpReq()` | fresh Workflow request | `CONTENT_TYPE_NONE`, fresh cache |
| `HttpReq(HttpRequest&&)` | transferred Workflow request | `CONTENT_TYPE_NONE`, fresh cache |

This makes `body()`, `form_kv()`, `form()`, and `json()` safe immediately after
either constructor. `content_type()` deterministically returns
`CONTENT_TYPE_NONE` until classification.

## HttpReq move construction

Move construction transfers:

1. Workflow `HttpRequest` base state;
2. `content_type_`;
3. the unique request cache;
4. route match/full paths;
5. route and query parameter maps;
6. cookie map and `cookies_parsed_` flag;
7. multipart boundary/parser settings;
8. header map;
9. parsed URI state.

The source cache becomes null. The source remains safely destructible and can
be the destination of a later move assignment.

## HttpReq move assignment

Move assignment first checks `this == &other`. A self-move returns immediately
without invoking the base operation or moving any member.

For distinct objects, the Workflow base assignment runs first, then every
derived member is move-assigned. Assigning the unique pointer destroys the
destination's old cache before taking the source cache. No manual delete or
source nulling is needed.

The operation is not declared `noexcept`, matching the existing API and the
potentially throwing string/map moves under arbitrary allocators.

## Response initialization and moves

`HttpResp::user_data` receives a default member initializer of `nullptr`. This
applies to both `HttpResp()` and `HttpResp(HttpResponse&&)`.

Move construction transfers:

- Workflow response parser and output blocks;
- wfrest header map;
- the opaque `user_data` value;
- response cookies.

After a distinct-object transfer, source `user_data` becomes null. Move
assignment performs an early self check; a self-move preserves base state,
headers, cookies, and `user_data` byte-for-byte.

The pointer remains non-owning: neither destructor nor overwrite deletes it.

## Moved-from contract

Moved-from request and response objects are:

- safely destructible;
- valid destinations for a subsequent move assignment;
- safe for operations whose documented preconditions do not require retained
  payload state.

Ordinary request cache access before reassignment is outside the contract after
the cache has transferred. This matches standard moved-from semantics and
avoids an allocation inside move operations.

## Compatibility

No public signature changes. Replacing a private pointer with `unique_ptr` does
not change ownership visible to callers. Existing proxy logic that moves only
the Workflow base subobject is unaffected; existing full response moves retain
their transfer behavior with deterministic `user_data` initialization.

Code that accidentally relied on an indeterminate default `user_data` value or
on state destruction after self-move had undefined behavior and is intentionally
corrected.

## Test plan

### Request unit tests

- default construction: empty cache and `CONTENT_TYPE_NONE`;
- placement-filled `HttpRequest` wrapping: safe cache access and destruction;
- move construction transfers body, content type, routes, query values, and
  parsed cookies;
- move assignment replaces an already populated destination;
- repeated move assignment releases all former cache allocations under LSan;
- self-move preserves cache and derived fields;
- moved-from request accepts a new request through move assignment.

### Response unit tests

- placement-filled default and base-wrapping constructors set `user_data` null;
- move construction transfers base status/body, headers, cookies, and
  `user_data`, then clears only the source pointer;
- move assignment replaces populated destination state;
- self-move preserves every observable field.

### Validation

- reproduce the original four failures on the parent `test` revision;
- run the focused suite under AddressSanitizer, UndefinedBehaviorSanitizer, and
  LeakSanitizer;
- compile touched production code with C++11 and
  `-Wall -Wextra -Wpedantic -Werror`;
- run focused CTest and the complete CTest suite;
- run `git diff --check`.
