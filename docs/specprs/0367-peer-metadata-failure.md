# Deterministic peer metadata failure handling

Issue: #367

## Summary

Make `HttpServerTask::peer_addr()` and `peer_port()` validate peer lookup and
address conversion before reading or returning socket metadata.

## Why

Both helpers currently allocate an uninitialized `sockaddr_storage`, call
Workflow's `get_peer_addr()`, ignore its return value, and then inspect
`ss_family`. Workflow documents failure through the normal socket-style
contract: it returns `-1`, sets `errno`, and does not promise to write the
output buffer.

A disconnected-task probe makes that contract visible:

```text
result=-1 errno=107 unchanged=1 len=128
peer_addr=Unknown peer_port=0
```

The address buffer remained byte-for-byte unchanged. The apparent fallback
values are therefore accidental: the wrapper reads an indeterminate family
and reaches its fallback only when those bytes do not happen to equal
`AF_INET` or `AF_INET6`.

`peer_addr()` has a second unchecked operation. If `inet_ntop()` fails, its
output buffer is unspecified, but the current implementation still constructs
and returns a `std::string` from that buffer.

## Goals

- Never inspect peer address storage after `get_peer_addr()` fails.
- Never read a family-specific structure shorter than that structure.
- Never construct a string from a failed `inet_ntop()` output.
- Return stable fallback values for unavailable or malformed peer metadata.
- Preserve successful IPv4 and IPv6 output.
- Keep the public API and ABI unchanged.

## Non-goals

- Changing Workflow's `get_peer_addr()` contract.
- Exposing lookup or conversion errors through a new public API.
- Adding host-name resolution, IPv6 scope formatting, or Unix socket support.
- Changing access-log format.
- Retaining `errno` across the convenience wrappers.

## Failure contract

The convenience methods use these existing fallback values:

```text
peer_addr() -> "Unknown"
peer_port() -> 0
```

They return the fallback whenever:

- `get_peer_addr()` returns a nonzero result;
- the returned address family is unsupported;
- the returned length is shorter than the structure required by its family;
- address-to-text conversion fails.

The methods do not interpret any bytes from the output address when lookup
fails.

## Successful address handling

For `AF_INET`, require at least `sizeof(sockaddr_in)` bytes, pass `sin_addr` to
`inet_ntop(AF_INET, ...)`, and convert `sin_port` with `ntohs()`.

For `AF_INET6`, require at least `sizeof(sockaddr_in6)` bytes, pass `sin6_addr`
to `inet_ntop(AF_INET6, ...)`, and convert `sin6_port` with `ntohs()`.

An `INET6_ADDRSTRLEN` buffer is sufficient for both textual forms. The text is
returned only when `inet_ntop()` returns a non-null pointer.

## Initialization policy

Value-initialize `sockaddr_storage` as defense in depth, but do not use
initialization as a substitute for checking the lookup result. A zeroed family
would make common failures appear deterministic while still hiding a broken
producer/consumer contract.

The text buffer does not need a sentinel value because it is read only after a
successful conversion. Its capacity is passed directly to `inet_ntop()`.

## Implementation plan

1. Value-initialize the local peer address in both public helpers.
2. Return the method-specific fallback when `get_peer_addr()` fails.
3. Validate the returned length before each family-specific cast and read.
4. Use const pointers because neither helper mutates the returned address.
5. In `peer_addr()`, select the binary address by family and call
   `inet_ntop()` once.
6. Return `"Unknown"` on unsupported families or conversion failure.
7. Add regression coverage for a task with no connected peer.

## Verification plan

### Contract probe

- construct a server task without a connected target;
- fill a caller-owned `sockaddr_storage` with a known byte pattern;
- call the inherited `get_peer_addr()`;
- verify `-1`, `ENOTCONN`, unchanged bytes, and unchanged capacity.

### Regression test

- call both convenience methods on a disconnected server task;
- verify repeated address calls return `"Unknown"`;
- verify repeated port calls return zero;
- run under AddressSanitizer and UndefinedBehaviorSanitizer.

The behavioral test fixes the public failure contract. The contract probe and
code-path review establish why the pre-fix implementation's same-looking
output is not safe: its result depends on an uninitialized read.

### Build and regression

- strict C++11 production and C++14 test warning-as-error builds;
- focused sanitizer execution of the peer metadata regression;
- full registered CTest suite, remaining at 38 targets when the case is added
  to the existing `HttpMsg_unittest` executable;
- cached `git diff --check`.

## Compatibility

No signatures, class layout, symbols, or successful wire-visible values
change. Connected IPv4 and IPv6 callers keep receiving the same numeric host
address and host-order port.

Previously undefined failure paths become deterministic. `"Unknown"` and zero
are already the methods' established unsupported-family fallbacks.

## Risks

A Workflow implementation that reports a family-specific address with a
nonstandard short length will now receive the fallback instead of being read
past its declared extent. That is intentional defensive behavior.

The two methods still perform separate peer lookups, so a connection state
change between calls may produce an address from one instant and a fallback
port from another. Providing an atomic address/port snapshot would require a
new API and is outside this correction.

## Acceptance criteria

- No address bytes are read after failed lookup.
- IPv4 and IPv6 reads require sufficient returned length.
- Text output is read only after successful conversion.
- Disconnected tasks return `"Unknown"` and zero repeatedly.
- Public API and successful connected behavior are unchanged.
- Strict builds, focused sanitizers, all 38 registered tests, and diff checks
  pass.
