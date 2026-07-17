# Safe HTTP Keep-Alive request policy

Issue: #361

## Summary

Resolve client `Keep-Alive` parameters with strict, saturating decimal parsing
and apply them only as restrictions on the server's configured connection
lifetime. No client-controlled value may trigger signed overflow, turn a
negative value into the hard maximum, or extend the server timeout.

## Why

`HttpServerTask::message_out()` currently parses `timeout` and `max` with
`atoi()`. It then multiplies the timeout by 1000 in signed `int` and only after
that casts it to unsigned for a hard-limit comparison.

The current path has several observable failures:

```text
timeout=5        -> 5000 ms
timeout=-1       -> 300000 ms
timeout=garbage  -> 0 ms
max=1 on seq=0   -> connection remains open
```

`timeout=2147484` triggers UBSan signed-integer overflow at the multiplication.
Longer inputs also rely on `atoi()` behavior outside the representable `int`
range. Prefixes such as `5junk` are accepted as 5.

The parsed timeout replaces the configured server timeout. A server configured
for a one-second idle lifetime can therefore be raised by a request to the
five-minute WFRest ceiling.

## Goals

- Parse recognized numeric values without exceptions or undefined behavior.
- Require a complete, unsigned ASCII decimal grammar.
- Distinguish malformed input from explicit zero.
- Saturate oversized valid decimals before arithmetic or conversion.
- Preserve the existing 300-second WFRest hard ceiling.
- Never allow a client hint to increase the normalized server timeout.
- Apply `max` to the actual one-based request count.
- Keep unknown extension parameters forward-compatible.
- Make policy behavior directly unit-testable without a live socket.

## Non-goals

- Changing Workflow's upstream HTTP task implementation.
- Emitting a response `Keep-Alive` header with timeout/max parameters.
- Adding a public Keep-Alive configuration API.
- Changing request `Connection` or response `Connection` precedence.
- Supporting quoted numeric values, signs, decimal fractions, or units.
- Removing the existing five-minute hard ceiling.
- Changing the server's configured timeout unit from milliseconds.

## Internal policy interface

Introduce an internal helper equivalent to:

```cpp
int resolve_keep_alive_timeout(const std::string& header_value,
                               long long connection_sequence,
                               int configured_timeout_ms);
```

The result is the idle timeout in milliseconds. Zero means close after the
current response. The helper is internal to WFRest and does not change the
installed public API.

`connection_sequence` is the zero-based sequence assigned by Workflow to the
current request on its connection.

## Server timeout normalization

Before request parameters are considered:

| Configured value | Normalized timeout |
| ---: | ---: |
| `0` | `0` |
| `1..300000` | unchanged |
| greater than `300000` | `300000` |
| negative | `300000` |

Workflow uses negative timeouts for an unlimited value, but WFRest already
applies a 300000 ms hard ceiling after request processing. Mapping a negative
configuration to that ceiling preserves the effective WFRest limit without an
unsigned cast.

An empty or absent `Keep-Alive` value returns this normalized timeout.

## Parameter grammar

Split the field value on commas. Each segment is considered independently.
Optional whitespace (`OWS`) is space or horizontal tab and is allowed around
the parameter key, equals sign, and value:

```text
OWS key OWS "=" OWS value OWS
```

Recognized keys are ASCII case-insensitive `timeout` and `max`. Unknown keys
and segments without a recognized key are ignored.

A recognized value is valid only when it is a non-empty sequence of ASCII
digits `0` through `9`. The entire trimmed value must match. Therefore the
following are malformed and ignored:

```text
timeout=
timeout=-1
timeout=+1
timeout=1.5
timeout=5junk
timeout=5=6
timeout=1 0
```

Malformed recognized parameters do not imply zero and do not consume the
key's first-valid slot. A later valid occurrence may apply:

```text
timeout=bad, timeout=5
```

The first valid occurrence of each recognized key wins. Later duplicates are
ignored. Parameter order does not otherwise affect the result.

## Saturating decimal parsing

Accumulate a value digit by digit with a caller-supplied ceiling. Before
evaluating `value * 10 + digit`, compare against the remaining room. Once the
ceiling would be exceeded, retain the ceiling while continuing to validate
that every remaining byte is a digit.

This separates two cases:

- a very long all-digit value is valid and saturated;
- a very long value ending in a non-digit is malformed and ignored.

No intermediate exceeds its ceiling, and no `strto*`, exception, errno, or
out-of-range cast is needed.

## `timeout` policy

Parse `timeout` as seconds with a saturation ceiling of 300. Multiplication by
1000 is then bounded and safe.

Apply the request value as:

```text
result_ms = min(normalized_server_ms, requested_seconds * 1000)
```

Consequences:

- `timeout=0` closes after this response;
- `timeout=5` reduces a 60-second server timeout to 5 seconds;
- `timeout=300` leaves a 60-second server timeout at 60 seconds;
- `timeout=999999...` saturates to 300 seconds but still cannot increase the
  server timeout;
- negative/signed/malformed values are ignored.

## `max` policy

Parse `max` as a request-count ceiling, saturating at `UINT64_MAX`.

Workflow connection sequences are zero-based. Convert the current sequence to
a one-based request ordinal:

```text
ordinal = max(connection_sequence, 0) + 1
```

The conversion is performed in `uint64_t`; a nonnegative signed 64-bit maximum
plus one remains representable. A defensive negative sequence is treated as
the first request.

If `ordinal >= max`, return zero. Examples:

| Sequence | Current ordinal | `max` | Result |
| ---: | ---: | ---: | --- |
| `0` | `1` | `0` | close |
| `0` | `1` | `1` | close |
| `0` | `1` | `2` | keep current timeout |
| `4` | `5` | `5` | close |
| `4` | `5` | `6` | keep current timeout |

An oversized all-digit `max` saturates and does not close ordinary connection
sequences. A malformed `max` is ignored.

## Integration

`HttpServerTask::message_out()` retains its existing decision about whether
the response is eligible for persistence:

- an explicit response `Connection` header controls eligibility;
- otherwise the request's keep-alive state controls eligibility;
- an ineligible response sets timeout zero without consulting the helper.

For an eligible response, call the helper with the captured request
`Keep-Alive` value, or an empty string if no such field was captured. Replace
the current `atoi()`, flag, split-vector, multiplication, comparison, and
unsigned hard-limit block with the returned timeout.

The existing response `Connection: close` versus `Connection: Keep-Alive`
selection then consumes the resolved zero/nonzero value unchanged.

## Implementation plan

1. Add an internal `HttpKeepAliveUtil.h` with ASCII OWS, case-insensitive key,
   and saturating decimal helpers.
2. Implement the timeout resolver as a small inline internal policy function.
3. Normalize configured values explicitly before parsing request parameters.
4. Track first valid `timeout` and `max` occurrences independently.
5. Apply timeout by minimum and max by one-based ordinal comparison.
6. Replace the ad hoc block in `HttpServerTask::message_out()`.
7. Add focused tests to the existing header/policy unit-test executable.

## Verification plan

### Decimal grammar

- zero and ordinary digits;
- empty, sign, fraction, embedded OWS, suffix, second equals, and binary NUL;
- extremely long digits;
- extremely long digits followed by an invalid suffix.

### Timeout policy

- exact seconds-to-milliseconds conversion;
- explicit zero;
- shorter client hint;
- equal and longer hints;
- saturated huge hint;
- negative and malformed values ignored;
- configured zero, negative, within-ceiling, and above-ceiling values.

### Duplicate and extension parameters

- case-insensitive keys and surrounding OWS;
- unknown parameters ignored;
- malformed recognized value followed by valid value;
- first valid duplicate wins;
- timeout and max in both orders.

### Max policy

- `max` zero, one, current ordinal, and next ordinal;
- zero-based sequence conversion;
- defensive negative sequence;
- signed-maximum sequence;
- saturated oversized max.

### Regression

- strict C++11 production and C++14 test warning-as-error compilation;
- focused ASan and UBSan run;
- the original overflow probe no longer has an analogous unsafe expression;
- full registered CTest suite.

## Compatibility

Public signatures and the hard ceiling remain unchanged. The following invalid
or unsafe behaviors intentionally change:

- malformed recognized values are ignored instead of becoming zero;
- negative timeouts are ignored instead of becoming five minutes;
- trailing garbage is rejected instead of accepting a numeric prefix;
- request timeouts cannot extend server configuration;
- `max=1` closes after the first response rather than the second.

Valid zero, positive decimal, unknown-extension, and case-insensitive key
behavior remains supported.

## Risks

Applications that relied on a client raising the server timeout will see the
configured timeout enforced. That behavior is a policy correction: a request
hint must not override an operator's connection-lifetime ceiling.

Treating `max` as a one-based request count changes the old zero-based
comparison by one response. This matches the parameter meaning and Workflow's
documented sequence assignment.

## Acceptance criteria

- No request value reaches `atoi()` or signed timeout multiplication.
- Every parse is complete, ASCII-only, and saturation-safe.
- Client parameters can only preserve or reduce the normalized server timeout.
- `max` closes on the stated one-based request ordinal.
- Malformed values never masquerade as explicit zero.
- Strict builds, focused sanitizers, and all 37 registered tests pass.
