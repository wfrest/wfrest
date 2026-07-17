# Self-initializing, fork-safe CurrentThread cache

Issue: #364

## Summary

Make every `CurrentThread` accessor initialize a consistent TID/text cache and
invalidate inherited thread-local cache state in the child of `fork()`.

## Why

The cache currently starts with these independent values:

```text
t_cached_tid   = 0
t_tid_str      = zero-initialized bytes
t_tid_str_len  = 6
```

Only `tid()` calls `cacheTid()`. Direct `tid_str_len()` followed by
`tid_str()` therefore reports length 6 for an empty string. Once initialized,
all three TLS values are copied into a forked child. Since the cached integer
is nonzero, the child never refreshes it and permanently reports its parent's
TID.

Fresh-access and fork probes reproduce both failures deterministically.

## Goals

- Make `tid()`, `tid_str()`, and `tid_str_len()` self-initializing.
- Preserve one coherent integer, string, and length tuple per thread.
- Preserve the existing `%5d ` text format.
- Reset inherited cache state in the child of `fork()`.
- Avoid a `getpid()` or `gettid()` syscall on every cached accessor.
- Keep the child reset handler async-signal-safe.
- Cover fresh threads and fork behavior with deterministic tests.

## Non-goals

- Changing the public namespace or accessor signatures.
- Changing the cached TID type or text buffer size.
- Supporting non-Linux platforms in this change.
- Adding process IDs, thread names, or logging APIs.
- Detecting raw `clone()` operations that bypass `pthread_atfork`.

## Cache invariant

Before initialization:

```text
t_cached_tid == 0
t_tid_str[0] == '\0'
t_tid_str_len == 0
```

After any public accessor returns:

```text
t_cached_tid == gettid()
t_tid_str_len == strlen(t_tid_str)
atoi(t_tid_str) == t_cached_tid
t_tid_str[t_tid_str_len] == '\0'
```

The formatted text remains `snprintf(buffer, size, "%5d ", tid)`. Width five
is a minimum, so larger TIDs legitimately produce a longer string.

## Common lazy initializer

`cacheTid()` remains the single initializer. Every public accessor calls it
before returning. Once `t_cached_tid` is nonzero, it returns without another
TID syscall.

The text length begins at zero instead of the unrelated constant six. This
makes internal state coherent even before an accessor is called.

## Fork handling

Register one `pthread_atfork()` child callback process-wide. Registration is
performed through `pthread_once()` when the thread cache is initialized for
the first time.

The child callback runs after libc has restored the child-side pthread state
and resets only the calling thread's inherited TLS:

```text
t_cached_tid = 0
t_tid_str[0] = '\0'
t_tid_str_len = 0
```

It does not allocate, lock, format, call `gettid()`, or use C++ library
objects. These scalar/byte stores are suitable for the post-fork child
context.

The child's next accessor follows normal lazy initialization and caches its
actual TID. Other parent threads do not exist in the child, so only the TLS of
the thread that called `fork()` needs resetting.

## Registration behavior

Declare an internal registration function from `SysInfo.cc` and call it only
inside the uncached branch of `cacheTid()`. `pthread_once()` guarantees that
concurrent first use from multiple threads installs at most one handler.

If a process forks before any `CurrentThread` accessor, no cache exists to go
stale; the first child accessor registers and initializes normally.

## Implementation plan

1. Initialize `t_tid_str_len` to zero.
2. Add an internal child reset callback in `SysInfo.cc`.
3. Add a `pthread_once_t` and registration function around
   `pthread_atfork(nullptr, nullptr, child_reset)`.
4. Invoke registration before filling a previously empty cache.
5. Make the two text accessors call `cacheTid()` like `tid()`.
6. Add `SysInfo_unittest` to the registered unit-test list.
7. Test direct text access, thread-local values, and a parent-cache/fork/child
   refresh sequence.

## Verification plan

### Fresh access

- call `tid_str_len()` before `tid()`;
- verify positive length equals `strlen(tid_str())`;
- verify the text parses to `tid()` and ends with a space.

### Per-thread behavior

- initialize from a worker thread;
- compare cached TID with the direct Linux TID;
- verify text/length consistency in that thread;
- verify worker and main TIDs differ.

### Fork behavior

- cache the parent TID before `fork()`;
- in the child compare `tid()` with direct `gettid()`;
- verify child text and length are coherent;
- exit using `_exit()` without invoking the test framework;
- in the parent verify normal child exit and unchanged parent cache.

### Build and regression

- strict C++11 production and C++14 test warning-as-error builds;
- focused ASan and UBSan test run;
- ThreadSanitizer is not required because all mutable cache fields are TLS and
  handler registration is protected by `pthread_once()`;
- full registered CTest suite (increasing from 37 to 38 tests).

## Compatibility

The returned TID and formatted representation do not change for callers that
previously invoked `tid()` first. Direct text access now returns initialized
data. Forked children now report the child TID, which is the intended semantic
correction.

The implementation adds a pthread at-fork handler on first cache use. WFRest
already links pthread through Workflow and its test/runtime targets.

## Risks

At-fork callbacks share process-global registration order. The child handler
performs only independent TLS stores and does not depend on locks or other
handlers, minimizing interaction risk.

`pthread_atfork()` registration failure is not meaningfully recoverable through
the existing void cache API. Normal cache initialization still works; fork
refresh depends on successful platform registration.

## Acceptance criteria

- Direct text/length access is coherent without a prior `tid()` call.
- Cached access avoids repeated TID syscalls.
- A child after `fork()` reports its own TID and coherent text.
- The public format and signatures remain unchanged.
- Strict builds, focused sanitizers, and all 38 registered tests pass.
