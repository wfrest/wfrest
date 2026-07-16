# Thread-safe FileCache capacity invariants

Status: Proposed

Issue: [#301](https://github.com/wfrest/wfrest/issues/301)

Integration branch: `test`

## Motivation

`FileCache` sits on the concurrent static-file serving path and advertises a
configurable memory bound. The current implementation does not make that bound
or all shared state race-free:

- `enabled_` has an unlocked read racing with locked writes.
- `max_cache_size_` is changed without locking and shrinking does not evict.
- eviction does not reserve space for the incoming entry, so insertion can
  leave `current_size_` above the configured maximum.
- one entry larger than the maximum is still retained.
- hits hold the one cache mutex across `stat` and output copying.
- stale entries remain resident after every failed validity check.

These are correctness issues under load and also suppress the throughput gain
that a memory cache is intended to provide.

## Required invariants

1. `current_size_` equals the sum of the content sizes of every entry in
   `cache_` while `mutex_` is held.
2. After any public mutating call returns,
   `current_size_ <= max_cache_size_`.
3. Cache map, current size, and maximum size are read or written only while
   holding `mutex_`.
4. The lock-free enabled fast path uses atomic operations; map access still
   repeats the enabled check under `mutex_`.
5. A published `CachedFile` snapshot is never mutated. Replacement publishes
   a new `shared_ptr` instead.
6. Stale-snapshot cleanup erases an entry only if the map still points to the
   snapshot that was validated, so an older reader cannot erase a newer write.

## Proposed design

### Race-free enable state

Represent `enabled_` as `std::atomic<bool>`. `get_file` may return immediately
after an acquire load observes `false`, preserving the disabled fast path
without a data race. `enable` and `disable` still take `mutex_` before storing
the new state so snapshot acquisition is ordered with configuration changes.
Operations that acquire the mutex repeat the flag check before touching the
map.

An operation that already acquired a shared snapshot may finish after a
subsequent disable or clear. This is intentional in-flight operation behavior;
the configuration change applies to new snapshot acquisition.

### Immutable shared snapshots

`add_file` constructs a complete `CachedFile` before taking the cache mutex.
Under the lock it removes any prior entry, enforces capacity, and publishes the
new `shared_ptr` in one step. It never edits an already published snapshot.

`get_file` and `is_valid` copy the `shared_ptr` while holding `mutex_`, then
release the lock before filesystem metadata access and content copying. The
shared pointer keeps the snapshot alive if another thread replaces or clears
the map.

### Strict capacity reservation

Replacement removes the old entry from byte accounting before considering the
new size. An entry larger than `max_cache_size_` is rejected; if it replaces an
existing key, the old value remains removed because it no longer represents
the caller's content.

For an admissible incoming size, eviction computes the maximum bytes that can
remain before insertion:

```text
available_before_insert = max_cache_size - incoming_size
retention_target = min(available_before_insert, 75% of max_cache_size)
```

Entries are removed until `current_size_ <= retention_target`. Computing 75%
as `max - max / 4` avoids multiplication overflow. The existing arbitrary
unordered-map eviction order remains unchanged; implementing LRU is separate
work.

`set_max_size` takes `mutex_`, updates the limit, and invokes the same eviction
routine with an incoming size of zero. A zero-byte limit clears all entries.

### Stale snapshot cleanup

Validity requires all of the following:

- `stat` succeeds and identifies a regular non-negative-size file;
- file size equals the snapshot content size;
- second-resolution modification time exactly equals the captured value.

Exact equality avoids accepting a replacement with an older timestamp, and
the size check detects same-second changes that alter length. Portable
nanosecond timestamp support remains out of scope.

When validation fails, reacquire `mutex_` and erase only when the key still
maps to the same `shared_ptr`. Update `current_size_` in the same critical
section.

## Public behavior

- Public method signatures do not change.
- Range reads remain half-open `[start, end)` and clamp the end to the cached
  content size.
- `set_max_size` now enforces its documented limit immediately.
- An oversized item produces a cache miss instead of consuming more than the
  configured cache.
- Disabling prevents new entries and new snapshot acquisitions; existing map
  contents remain available after re-enabling.

## Test plan

Add a dedicated `FileCache_unittest` to CTest with isolated setup/teardown and
cover:

1. half-open range reads and end/start clamping;
2. exact byte accounting on same-key replacement;
3. insertion eviction with an incoming-size reservation;
4. oversized insertion and oversized replacement rejection;
5. immediate enforcement when the maximum shrinks, including zero;
6. same-timestamp size changes and deleted-file stale eviction;
7. disabled add/get behavior;
8. concurrent add/get/enable/disable/resize stress.

Run the focused test under AddressSanitizer and UndefinedBehaviorSanitizer,
run a ThreadSanitizer build when supported by the host, and run the complete
functional CTest suite.

## Risks and mitigations

- **More snapshot allocations on replacement.** This moves string copying out
  of the global critical section, favoring concurrent hit latency. The cache
  already allocates one shared object per key.
- **Transient miss during a concurrent replacement.** An older reader that
  detects staleness returns a miss but cannot erase the newer snapshot. The
  next lookup observes the replacement.
- **Second-resolution timestamps.** Exact time plus size is strictly stronger
  than the current `current <= cached` check. Nanosecond portability can be
  added independently.
- **Eviction remains non-LRU.** The size invariant is independent of policy;
  policy changes require access-order metadata and separate benchmarking.

## Rollback

Revert the implementation and its dedicated test registration together. No
wire format, persisted cache format, or public source signature is changed.
