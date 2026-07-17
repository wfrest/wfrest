# Alias-safe bounded streaming for Compressor

Issue: #349

## Summary

Rework `Compressor::gzip()` and `Compressor::ungzip()` so input is consumed in
zlib-sized chunks, output is staged independently, and a destination is not
mutated until the operation succeeds or has definitively failed. This makes
the existing overloads safe for source/destination aliasing, empty payloads,
null boundary inputs, and lengths wider than zlib's per-call counters.

## Why this change is needed

The current implementation begins every raw operation with `dest->clear()`.
The string overload obtains `src->c_str()` and then delegates to that raw
operation. When `src == dest`, or when raw `data` points anywhere into the
destination string's storage, clearing the destination changes or invalidates
the bytes zlib is about to read.

Observed results include:

```text
gzip(&value, &value):       StatusOK, but decoded data differs
ungzip(&value, &value):     StatusUncompressError, source lost
gzip(value.data(), ..., &value): StatusOK, but decoded data differs
gzip(empty):                StatusCompressError
gzip(nullptr_string, ...):  UBSan null-member call
gzip(..., nullptr_dest):    UBSan null-member call
```

The same implementation narrows the complete `size_t` length into
`z_stream::avail_in`, whose type is `uInt`. Decompression also allocates
`len * 2` bytes before zlib reports a single output byte. The former silently
truncates wide inputs; the latter can overflow or allocate based on an
unrelated compressed-size guess.

## Goals

- Support string source/destination identity.
- Support a raw input range backed by the destination string.
- Validate null object pointers and pointer/length combinations.
- Generate a valid gzip member for an empty payload.
- Preserve the existing successful empty result for zero-byte ungzip input.
- Feed zlib without narrowing the complete input size.
- Grow output from actual produced bytes rather than predicted decompressed
  size.
- Preserve failure status codes and valid-destination clearing behavior.
- Add deterministic unit coverage and sanitizer verification.

## Non-goals

- Adding compression formats beyond gzip/zlib auto-detection already used by
  the implementation.
- Adding a decompressed-size or compression-ratio policy limit.
- Changing compression level, gzip headers, or zlib window settings.
- Decoding multiple concatenated gzip members.
- Rejecting trailing bytes after the first completed stream if zlib currently
  completes the first stream successfully.
- Changing public signatures or introducing new status codes.
- Catching allocation exceptions.

## Public API contract

The existing overloads remain unchanged:

```cpp
static int gzip(const std::string *src, std::string *dest);
static int gzip(const char *data, size_t len, std::string *dest);
static int ungzip(const std::string *src, std::string *dest);
static int ungzip(const char *data, size_t len, std::string *dest);
```

### Object pointer validation

| Condition | gzip result | ungzip result | Destination effect |
| --- | --- | --- | --- |
| `dest == nullptr` | `StatusCompressError` | `StatusUncompressError` | none possible |
| string `src == nullptr`, valid `dest` | `StatusCompressError` | `StatusUncompressError` | cleared |
| raw `data == nullptr`, `len > 0` | `StatusCompressError` | `StatusUncompressError` | cleared |
| raw `data == nullptr`, `len == 0` | valid empty gzip | successful empty output | replaced with result |

No invalid object pointer is dereferenced.

### Aliasing

All of the following are supported:

```cpp
Compressor::gzip(&value, &value);
Compressor::ungzip(&value, &value);
Compressor::gzip(value.data(), value.size(), &value);
Compressor::ungzip(value.data(), value.size(), &value);
```

The raw input range may be the whole destination buffer or a valid subrange.
The source bytes must remain readable for the requested length when the call
begins, as with any pointer/length API.

### Destination transaction

- zlib output is accumulated in independent local storage.
- `dest` is not modified while zlib can still read source bytes.
- on `StatusOK`, the complete result replaces `dest` exactly once;
- on any error, a valid `dest` is cleared before return;
- stale destination content is never appended to a new result.

This transaction applies to validation errors, zlib initialization errors,
stream errors, malformed/truncated input, and cleanup errors.

### Empty data

`gzip(nullptr, 0, dest)` and gzip of an empty string return `StatusOK` and
produce a non-empty valid gzip member. Passing that member to `ungzip()`
returns `StatusOK` and an empty destination.

For compatibility, `ungzip(nullptr, 0, dest)` and ungzip of an empty string
continue to return `StatusOK` with an empty destination even though no encoded
member is present.

### Binary data

Input is always a byte range. Embedded NUL bytes and all other byte values are
preserved through gzip/ungzip round trips.

## Streaming model

### Input

Maintain a `size_t` source offset outside `z_stream`. Whenever zlib has
consumed its current input:

1. compute the remaining `size_t` length;
2. cap the next chunk at `std::numeric_limits<uInt>::max()`;
3. assign only that chunk to `next_in` and `avail_in`;
4. advance the external offset after assigning the chunk.

No cast to `uInt` occurs until the value is known to fit.

### Output

Use a fixed-size local output buffer. For each deflate/inflate step:

1. reset `next_out` and `avail_out` to the local buffer;
2. invoke zlib;
3. append exactly `buffer_size - avail_out` bytes to staged output;
4. repeat while the stream needs more output space or input.

Do not use `compressBound()` as a full-output allocation requirement and do
not seed decompression output with `len * 2` bytes.

### gzip completion

- Use `Z_NO_FLUSH` while assigned input remains or more external input exists.
- Use `Z_FINISH` only after all external input has been assigned and consumed.
- Continue until `Z_STREAM_END`.
- Empty input still performs the `Z_FINISH` cycle and therefore emits a valid
  member.

### ungzip completion

- Feed new chunks whenever `avail_in == 0` and external input remains.
- Append every produced byte.
- Succeed only on `Z_STREAM_END`, except for the explicit zero-byte
  compatibility case.
- Treat data, dictionary, memory, buffer/no-progress, and premature-input-end
  states as `StatusUncompressError`.
- Always call `inflateEnd()` after successful initialization.

## Implementation plan

1. Add internal helpers for clearing-and-returning the appropriate error and
   for assigning bounded input chunks.
2. Validate the string overloads before reading `c_str()` or `size()`.
3. Rewrite gzip around a fixed output block and external `size_t` input
   cursor; delay destination mutation until completion.
4. Rewrite ungzip with the same input/output model and an explicit
   no-progress/truncated-input guard.
5. Preserve zlib initialization parameters and return-code mapping.
6. Expand `Compress_unittest` from two happy paths into boundary, alias, and
   failure regressions.

## Verification plan

### Successful operations

- Existing short and long text round trips.
- Empty string and `(nullptr, 0)` gzip members round trip to empty.
- Binary bytes including embedded NUL and `0xFF` round trip exactly.
- A stale destination is replaced rather than appended.

### Aliasing

- In-place string gzip followed by in-place string ungzip restores the
  original value.
- Raw gzip whose source is the whole destination buffer restores exactly.
- Raw ungzip whose source is the whole destination buffer restores exactly.
- A valid raw subrange backed by the destination compresses only that
  subrange.

### Invalid and incomplete inputs

- Null string source clears a valid destination and returns the operation's
  error code.
- Null destination returns the operation's error code without a crash.
- Null raw data with non-zero length clears and fails.
- Malformed and truncated encoded data clear and fail.
- Zero-byte ungzip preserves its existing successful-empty behavior.

### Tooling and regression

- Re-run the pre-fix alias/null/empty probe under ASan and UBSan.
- Compile production and unit sources with strict warning-as-error flags.
- Run `Compress_unittest` and HTTP gzip integration tests under ASan/UBSan.
- Run the complete registered CTest suite.

## Risks and mitigations

### Infinite no-progress loops

Streaming APIs can return without consuming or producing bytes when they need
more input. Explicitly fail when all external input is exhausted, zlib has no
assigned input, no output was produced, and the stream has not ended.

### Extra copies

Staging output is necessary for alias safety, but it does not copy the input.
Fixed output blocks avoid the previous eager decompression allocation and
append only actual bytes.

### Behavioral changes at empty input

Empty gzip changes from error to a valid encoded member. This aligns the API
with gzip's data model and makes gzip/ungzip round trips total over all string
payloads. Empty ungzip retains its historical behavior.

## Acceptance criteria

- All string and raw alias reproductions round trip exactly.
- Empty payload gzip produces a valid member.
- Null object pointers do not trigger sanitizer findings.
- No full `size_t` input length is narrowed to `uInt`.
- Decompression output is based on produced bytes, not `len * 2`.
- All failures clear a valid destination and return the existing error code.
- Strict builds, focused sanitizer checks, and the full suite pass.
