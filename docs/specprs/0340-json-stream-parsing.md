# Exact and streaming JSON input parsing

Issue: #340

## Motivation

`Json::parse(FILE*)` currently seeks to the end, stores `ftell()` in a signed
length, allocates one buffer, and writes a terminator without checking any
operation. Non-seekable streams return `-1` from `ftell()`, which turns the
terminator write into a heap-buffer-underflow. Null streams and read failures
also return default `Json()`, which means valid JSON `null` rather than an
invalid parse result.

The bundled parser accepts only a NUL-terminated C string. Passing
`std::string::c_str()` without first validating the complete byte sequence lets
an embedded raw NUL hide every following byte. A valid prefix followed by NUL
and invalid data is therefore accepted.

## Goals

1. Parse every supplied JSON byte or reject the input.
2. Support seekable files and non-seekable `FILE*` streams safely.
3. Distinguish null/read-error/empty inputs from valid JSON `null`.
4. Remove negative-length arithmetic and manual FILE buffer allocation.
5. Preserve the existing rewind behavior for seekable `FILE*` inputs.

## Non-goals

- Replacing or extending the bundled JSON grammar.
- Adding a configurable document-size limit.
- Taking ownership of or closing a caller's `FILE*`.
- Changing JSON value construction or serialization.
- Rewinding `std::ifstream`, whose existing overload consumes its current
  stream position.

## Exact byte contract

All parsing entry points reject an input containing any raw NUL byte. This
check occurs in the private parsing constructor shared by:

- `Json::parse(const std::string&)`;
- `Json::parse(const std::ifstream&)`;
- `Json::parse(FILE*)`.

The check applies to the complete `std::string`, before passing its `c_str()` to
the bundled parser. JSON text containing the six ASCII characters `\u0000`
remains valid because it contains an escaped code point, not a raw NUL byte.

Empty input remains invalid. Whitespace and other trailing bytes keep the
bundled parser's existing grammar behavior, but no suffix can be hidden after
a NUL.

## Invalid parse result

Input failures return the same invalid representation produced by parsing an
empty string: `node_ == nullptr`, `allocated_ == false`, and
`is_valid() == false`.

This differs intentionally from default `Json()`, whose node represents valid
JSON `null`. The following `FILE*` cases return invalid:

- the pointer is null;
- no bytes are available before EOF;
- `fread()` sets the stream error indicator;
- `fread()` returns zero without either EOF or an error, preventing an
  unbounded retry loop;
- the complete bytes fail JSON or raw-NUL validation.

## FILE positioning

`Json::parse(FILE*)` first calls `fseek(fp, 0, SEEK_SET)`.

- On success, parsing begins at byte zero, preserving current behavior even if
  the caller positioned a regular file elsewhere. Successful parsing leaves
  the stream consumed at EOF.
- On failure, the implementation calls `clearerr(fp)` to remove the failed
  seek state, then reads from the stream's current position. This permits pipes,
  sockets, and other forward-only streams.

The parser never calls `fclose()` and never otherwise assumes ownership.

## Chunked read algorithm

The implementation uses a fixed-size stack buffer and one accumulated
`std::string`:

1. call `fread(buffer, 1, sizeof(buffer), fp)`;
2. append exactly the returned byte count when nonzero;
3. continue immediately after a full chunk;
4. after a short chunk, return invalid on `ferror(fp)`;
5. finish on `feof(fp)`;
6. continue after a nonzero short read with neither indicator;
7. return invalid after a zero-byte read with neither indicator.

This removes `SEEK_END`, `ftell`, signed size conversion, `malloc`, unchecked
terminator writes, and `fread` narrowing. The final accumulated string is sent
through the exact byte contract once.

## Compatibility

Valid JSON in regular files, ifstreams, and strings retains its parsed value.
Seekable FILE parsing still starts at byte zero. Valid forward-only streams are
newly supported.

Intentional behavior changes:

- null FILE pointers and read failures become invalid instead of JSON null;
- raw embedded NUL becomes invalid instead of truncating input;
- empty streams become invalid explicitly;
- non-seekable streams parse rather than corrupt memory.

## Implementation plan

1. Add raw-NUL validation to the private parsing constructor.
2. Replace FILE length probing and allocation with the chunk loop.
3. Use the private invalid representation on input failures.
4. Register a focused `Json_unittest` CTest target.
5. Cover exact string input, seekable rewind/multi-chunk input, null streams,
   valid and empty pipes, FILE raw NUL, and escaped `\u0000`.

## Validation plan

- reproduce the pre-fix pipe heap-buffer-overflow under ASan;
- reproduce pre-fix acceptance of valid-prefix/NUL/invalid-suffix input;
- strict C++11 production compile with
  `-Wall -Wextra -Wpedantic -Werror`;
- ASan/UBSan/LSan focused JSON tests;
- complete CTest suite;
- `git diff --check` and clean submodule/worktree audit.
