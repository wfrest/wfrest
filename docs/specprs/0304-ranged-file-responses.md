# Consistent and RFC-valid ranged file responses

Status: Proposed

Issue: [#304](https://github.com/wfrest/wfrest/issues/304)

Integration branch: `test`

## Motivation

The file API exposes half-open byte slices but translates them incorrectly to
HTTP and inconsistently across cache states. Today `[10, 20)` on a 100-byte
file advertises `bytes 10-20/10`, remains a 200 response, and can return either
10 or 100 bytes depending on whether `CachedFile` hit the cache.

RFC 9110 defines byte positions in Content-Range as inclusive, requires the
complete representation length, and gives Content-Range response semantics
only to 206 and 416. The current header is therefore invalid independently of
the cache body mismatch.

The implementation also narrows `size_t` offsets to `int`, which corrupts
valid offsets above `INT_MAX` and makes the documented negative-start behavior
depend on implementation-defined signed conversion.

## Goals

1. Preserve the public half-open `[start, end)` API and documented negative
   suffix starts.
2. Produce internally consistent status, Content-Range, and body bytes.
3. Make cache miss and cache hit behavior identical for a requested slice.
4. Keep all normalized sizes and offsets in `size_t` and support sparse files
   with offsets above 2 GiB.
5. Define safe behavior for empty files, out-of-bounds ends, invalid suffixes,
   short reads, and allocation failure.

## Non-goals

- Parsing a request `Range` field automatically.
- Supporting multiple ranges or multipart/byteranges.
- Replacing the current whole-range/whole-file asynchronous allocations with
  streaming.
- Changing any public function signature.

## Range model

The public methods continue to accept unsigned `size_t` parameters for source
compatibility. C++ conversion of a negative integer to `size_t` is defined
modulo `2^N`; the normalizer uses that representation directly instead of
narrowing back to `int`.

### Start decoding

- Values at or below `std::numeric_limits<make_signed<size_t>::type>::max()`
  are non-negative absolute offsets.
- Larger values encode a negative suffix start. Its magnitude is calculated
  with unsigned arithmetic as `~value + 1`.
- A zero magnitude or magnitude greater than the file size is invalid.
- A valid suffix start resolves to `file_size - magnitude`.

Real regular-file offsets come from signed `off_t`, so a valid absolute file
offset cannot overlap the unsigned suffix-encoding region.

### End decoding

- `size_t(-1)` means EOF, as in the existing API.
- Other unsigned values in the suffix-encoding region are invalid; negative
  ends are not documented.
- A non-negative explicit end beyond EOF is clamped to the file size, matching
  existing cached-hit slicing behavior.

### Empty and invalid ranges

For a zero-length file, `[0, EOF)` and `[0, 0)` represent the full empty file.
Other ranges are invalid. For a non-empty file, `start >= file_size` or
`end <= start` is invalid after normalization.

The normalized result stores `start`, exclusive `end`, and whether the slice
is partial (`start != 0 || end != file_size`).

## HTTP mapping

- Full content: retain/default to status 200 and do not generate
  Content-Range.
- Partial content: set status 206 and generate
  `bytes start-(end - 1)/file_size`.
- Invalid range: return `StatusFileRangeInvalid`, map it to status 416, and
  generate `bytes */file_size` before the error body is produced.

This matches RFC 9110 Sections 14.4 and 15.3.7. The API is an explicit
application slice rather than automatic request Range parsing, but once it
generates Content-Range the same response semantics apply.

## Shared preparation

Introduce one internal preparation path for `send_file` and
`send_cached_file` that:

1. performs one `stat` to verify a regular file and collect size/mtime;
2. normalizes the requested range;
3. resolves Content-Type;
4. sets 206/416 metadata when required.

This removes duplicated range and content-type logic and prevents cache and
non-cache drift.

## Asynchronous read behavior

### Uncached

Allocate exactly `end - start` bytes and check allocation before creating the
pread task. Associate the expected byte count with the callback. A failed or
short read becomes `StatusFileReadError`; partial-response metadata is removed
before generating the error response.

The full empty-file case succeeds synchronously without allocating or passing
a null buffer to the I/O task.

### Cached

When caching is disabled, delegate directly to the uncached path. On a cache
hit, send the normalized slice already returned by `FileCache`.

On a miss, read the complete file for cache population, but store normalized
`start` and `end` in the callback context. Validate that the complete read
finished, publish the full immutable cache snapshot, and append only
`[start, end)` to the response. This makes first miss and later hit identical.

An empty file can be added to the cache synchronously and returned with a zero
length body.

## Test plan

Extend the HTTP file integration suite to verify:

1. existing positive and negative half-open slices;
2. 206 status and inclusive/complete Content-Range values;
3. explicit end clamping at EOF;
4. invalid absolute, suffix, and empty ranges returning 416 plus
   `bytes */file_size`;
5. full non-empty and empty files returning 200 without Content-Range;
6. a four-byte tail above `INT_MAX` in a sparse file;
7. identical cached miss and hit status, header, and body bytes;
8. disabled-cache delegation.

Run the focused server test repeatedly, then run the complete CTest suite and
warning-enabled C++11 production build.

## Compatibility and risks

- Existing clients that relied on the malformed Content-Range value or status
  200 for a partial body will observe corrected wire metadata.
- Explicit ends beyond EOF now consistently clamp rather than causing a short
  uncached read with an overclaimed header.
- Existing negative start examples remain supported without public overload
  changes.
- Automatic request Range parsing remains absent, so applications still
  choose the slice passed to `File` or `CachedFile`.

## Rollback

Revert the shared normalizer, callback context changes, error-status mapping,
and integration tests together. Cache data is in-memory only and no persistent
format migration is required.
