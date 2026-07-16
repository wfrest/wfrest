# Owned and deterministic route-table matching

Issue: #322

## Motivation

The route tree stores `StringPiece` keys that watch the caller's buffer, even
though its comment claims to retain the route. A scoped `std::string` therefore
leaves both the route set and child maps with dangling pointers.

Search is also driven by map iteration rather than route semantics. The first
parameter child returns its recursive failure without backtracking, wildcard
and parameter priority depends on lexical spelling, short wildcard prefixes can
beat longer ones, and failed branches leak tentative parameter values.

Finally, route enumeration emits only leaves, so an endpoint disappears from
`all_routes()` as soon as a longer route is registered beneath it.

## Goals

1. Own every buffer observed by persistent `StringPiece` keys.
2. Give child nodes exception-safe single ownership.
3. Define stable static, parameter, and wildcard precedence.
4. Backtrack parameter candidates transactionally.
5. Leave output parameters unchanged after a failed lookup.
6. Return only nodes that contain registered handlers.
7. Enumerate every registered endpoint, including prefix endpoints.
8. Remove signed narrowing from path cursor arithmetic.

## Non-goals

- Changing route syntax or handler signatures.
- Rejecting ambiguous parameter patterns during registration.
- Making a wildcard segment non-terminal.
- Changing trailing-slash normalization performed by `Router::call`.

## Route storage ownership

`RouteTable` owns unique complete routes in `std::set<std::string>`. Node-based
set storage keeps each string buffer stable while registration parses it and
deduplicates repeated complete patterns.

Each `RouteTableNode` additionally owns the unique text of its direct child
segments in a node-local `std::set<std::string>`. Its `StringPiece` map keys
watch that local stable storage. This makes the public low-level
`RouteTableNode::find_or_create(StringPiece)` safe even when the caller's full
route buffer is temporary.

Member declaration order places each owning string set before the map that
watches it, and places the complete route store before the root. Reverse
destruction therefore destroys observers before their storage. Child maps keep
allocation-free `StringPiece` lookup and own nodes through
`std::unique_ptr<RouteTableNode>`. Manual recursive deletion is removed.

## Registration parsing

`find_or_create` uses `size_t` for cursor and anchor values. It checks
`cursor >= route.size()` before indexing. A single leading slash is skipped per
segment, and a final slash returns the current endpoint as before. The special
root route `/` remains stored as its own child so enumeration continues to
report `/`.

Duplicate complete routes reuse the same owned string and node path.

## Lookup phases

At each tree node, matching proceeds in explicit phases:

1. exact static child;
2. valid parameter children in lexical route-pattern order;
3. terminal wildcard children ranked by descending literal-prefix length, with
   lexical pattern order as the tie-break.

An endpoint is successful only when its `verb_handler_map` is non-empty.

### Endpoint and root handling

When `cursor == route.size()`:

- return the current node if it has a handler;
- otherwise allow a direct `*` child with a handler, capturing an empty match;
- otherwise fail.

The special `/` child is checked for the input `/`. If absent, normal segment
logic still permits a root `*` wildcard to capture the empty suffix.

### Static match

The segment between the current cursor and next slash is looked up exactly.
Failure beneath an exact child continues to the parameter phase at the parent;
this supports a parameter route when a static prefix exists but its deeper
subtree does not match.

### Parameter match

A parameter pattern begins with `{`, ends with `}`, and contains a non-empty
name after trimming spaces inside the braces. Empty input segments never match
parameters.

For each candidate:

1. remember whether the same parameter name already exists and copy its old
   value when present;
2. assign the current segment;
3. recursively search the child;
4. return on success;
5. on failure, restore the old value or erase the newly inserted name, then try
   the next candidate.

Lexical pattern order is the deterministic tie-break when more than one
parameter subtree can complete successfully.

### Wildcard match

A child pattern ending in `*` is terminal. Its literal prefix is the pattern
without the final asterisk. It is a candidate when the current segment starts
with that prefix and the child has a handler.

After all candidates are inspected, the longest prefix wins. Equal lengths
retain map order. The match path remains the current segment plus the untouched
remainder of the request path, preserving existing capture behavior.

Examples:

| Registered routes | Input | Winner |
| --- | --- | --- |
| `/files/static` and `/files/{name}` | `/files/static` | static |
| `/files/{name}` and `/files/*` | `/files/abc` | parameter |
| `/files/a*` and `/files/ab*` | `/files/abc` | `ab*` |
| `/u/{id}/posts`, `/u/{name}/profile` | `/u/x/profile` | second parameter subtree |

## Failed lookup outputs

The public `RouteTableNode::find` snapshots `route_params` and
`route_match_path`, calls a private recursive implementation, and restores both
snapshots if no endpoint is found. This applies even when callers provide
pre-existing parameter values.

Internal parameter branches still restore locally before trying siblings, so a
successful later branch contains only values from its own path.

## Enumeration

`all_routes` calls the visitor whenever the current node has handlers, then
recurses into children. It no longer treats `children_.empty()` as equivalent
to “this is an endpoint.”

Thus registering both `/api/v1` and `/api/v1/v2` reports both exactly once.
Nodes created only as structural prefixes are not emitted.

## Compatibility

Public method signatures and route syntax remain unchanged. Exact routes,
parameters, root `/`, root `/*`, and prefix wildcards retain their intended
forms.

Behavior changes where old results were unsafe or iteration-dependent:

- caller route buffers may now be destroyed safely;
- static routes consistently beat parameters, which beat wildcards;
- longer wildcard prefixes beat shorter prefixes;
- alternate parameter subtrees are tried after failure;
- prefix endpoints appear in enumeration;
- empty handler nodes are not reported as matches.

## Test plan

### Ownership and sanitizer tests

- register through both `RouteTable` and direct `RouteTableNode` calls from
  scoped and overwritten heap strings, churn the allocator, then find and
  enumerate routes;
- repeat insertion and destruction under ASan/UBSan;
- verify unique child ownership under LSan.

### Matching tests

- exact over parameter and wildcard;
- parameter over wildcard;
- longest wildcard prefix;
- parameter subtree backtracking;
- restoration of new and pre-existing same-name parameters;
- unchanged route-match string on failure;
- empty segment and empty-name patterns;
- root `/`, root `/*`, trailing slash, and no-handler nodes;
- long path segment arithmetic without signed narrowing.

### Enumeration and validation

- prefix and descendant endpoints both emitted once;
- duplicate registrations reuse a route path;
- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- focused tests under ASan and UBSan;
- complete CTest suite;
- `git diff --check`.
