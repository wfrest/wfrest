# Signed and terminal streaming push writes

Issue: #313

## Motivation

Workflow's server-task `push` API returns `int`: non-negative values report the
number of bytes written and `-1` reports an error through `errno`. wfrest stores
that result in `size_t` in both the first body write and the timer retry. A
negative result therefore becomes a large positive number and always satisfies
`nwritten >= 0`.

The subsequent unsigned subtraction is unsafe. If a 10-byte write fails with
`-1`, the remaining count becomes 11. The next `data.size() - nleft` wraps, and
the resulting pointer can precede the owned string buffer.

The same push flow also ignores the return value for the initial HTTP header,
does not notify the error callback for retry failures, and registers another
named condition after sending the terminal zero chunk.

## Goals

1. Keep signed socket results separate from validated byte offsets.
2. Use one result-accounting contract for headers, chunks, and retries.
3. Retry partial and would-block writes without losing or duplicating bytes.
4. Report every fatal path through the user callback at most once.
5. Stop the stream after its terminal zero chunk.
6. Preserve existing public `HttpResp::Push` callback signatures and chunk
   formatting.

## Non-goals

- Replaying named-condition signals that arrive before a condition is present.
- Adding queues, backpressure policies, or application-level event retention.
- Changing Workflow's `int push(const void *, size_t)` API.
- Defining retry limits for a connection that continuously reports
  `EAGAIN`/`EWOULDBLOCK`.

## Write accounting

An internal helper receives:

```text
total_size, current_offset, signed_write_result, captured_errno
```

It returns an action and a validated next offset. The helper never forms a
pointer and has no network dependency, so all transitions are unit-testable.

| Condition | Action | Next offset |
| --- | --- | --- |
| `current_offset > total_size` | fatal | unchanged |
| negative result with `EAGAIN` or `EWOULDBLOCK` | retry | unchanged |
| other negative result | fatal | unchanged |
| zero result with bytes remaining | fatal | unchanged |
| positive result greater than remaining bytes | fatal | unchanged |
| positive result smaller than remaining bytes | retry | offset + result |
| positive result exactly equal to remaining bytes | complete | total size |
| zero result when already complete | complete | total size |

The caller captures `errno` immediately after `push`, before allocation,
logging, callbacks, or task creation can overwrite it.

Each concrete `push` call is limited to `INT_MAX` bytes because the Workflow
return type cannot represent a larger successful count. A buffer larger than
that limit completes through multiple partial-write transitions.

## Owned retry state

Retry state owns the complete byte string and stores a validated offset, not an
unsigned remaining count. A retry pointer is formed only as:

```text
owned_data.data() + validated_offset
```

The requested length is `min(total_size - offset, INT_MAX)`. The state is
destroyed on completion or fatal failure. On retry it is reused and a one
millisecond timer is pushed to the front of the same server series.

## Ordering

For a non-terminal write, the next named conditional is registered only after
the current bytes have been written completely. A partial or would-block result
creates a retry timer at the front of the server series, and completion of the
last retry registers the condition.

The sequence is therefore:

```text
current condition -> zero or more front retries -> next condition
```

No later chunk can overtake an earlier partial chunk, and a fatal retry cannot
leave the server series waiting on a condition that will never be used. Signals
that arrive while a write is still retrying are not buffered; event replay is
explicitly outside this API's contract.

## Header writes

The serialized HTTP response header is passed to the same owned write path as
chunk data. A complete header registers the first named conditional. A partial
or would-block header owns and retries the unsent suffix, then registers the
first condition after completion. A fatal header write reports the error and
registers no condition.

The response is switched to `noreply` before this path so the normal Workflow
reply machinery cannot send a second header.

## Error callback

Push context stores a `failed` flag. Its failure operation is idempotent:

1. if already failed, return;
2. mark failed;
3. invoke the configured `PushErrorFunc` once.

The operation covers:

- a closed or non-keep-alive request discovered before a chunk;
- initial header failure;
- first chunk failure;
- timer-retry failure;
- invalid offset, zero progress, or an impossible over-reported byte count.

Any callback task that observes an already failed context returns immediately,
without invoking the error callback or registering another condition.

## Terminal chunk

The application `PushFunc` continues to communicate stream completion by
leaving its output string empty. wfrest serializes exactly:

```text
0\r\n\r\n
```

The terminal bytes use the normal complete/partial/retry/error path, but no
next named condition is registered. Later calls to `sse_signal` therefore do
not invoke this stream's callback or write bytes after the chunked message has
ended.

Non-empty data retains the current formatting:

```text
hex-size\r\n
data\r\n
```

## Compatibility

The two public `Push` overloads and callback types do not change. Header fields,
chunk encoding, condition names, and the one-millisecond retry interval remain
compatible.

Behavior changes intentionally on broken or terminal streams:

- signed errors can no longer corrupt offsets;
- fatal retry and closed-connection paths invoke the error callback once;
- empty callback output ends the stream instead of arming another condition;
- partial initial headers are retried instead of being truncated.

## Test plan

### Pure write-accounting tests

- complete full write;
- multiple partial writes and exact cumulative offsets;
- `EAGAIN` and `EWOULDBLOCK` with unchanged offsets;
- fatal negative result;
- zero progress with bytes remaining;
- zero at an already complete offset;
- over-reported result;
- invalid input offset;
- boundary values around `INT_MAX` without arithmetic overflow.

### HTTP integration

- start an SSE response with a unique condition name;
- signal one non-empty event and verify the decoded client body;
- signal an empty event and verify response completion;
- signal the name again after termination and verify the callback count remains
  two;
- verify the error callback was not called on the successful path.

### Static and dynamic validation

- compile the accounting helper and touched push code under C++11 with
  `-Wall -Wextra -Wpedantic`;
- run helper tests under AddressSanitizer and UndefinedBehaviorSanitizer;
- run the focused unit and HTTP tests;
- run the complete CTest suite;
- confirm `HttpMsg.cc` no longer emits the unsigned-comparison warnings at the
  affected push sites;
- run `git diff --check`.
