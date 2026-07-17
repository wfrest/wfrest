# Timezone-independent and reentrant HTTP date formatting

Issue: #346

## Summary

Add an explicit UTC formatting path to `Timestamp`, make calendar conversion
reentrant, and use the UTC path for HTTP response `Date` and cookie `Expires`
values. Protocol dates must describe the same instant regardless of the
process timezone and must retain English HTTP-date names regardless of the
process locale.

## Why this change is needed

`Timestamp::to_format_str()` currently passes `std::localtime()` directly to
`std::put_time()`. The HTTP call sites use that local-time API with a format
ending in the literal `GMT`.

This has three observable consequences:

1. A non-UTC process emits local wall-clock fields while claiming they are
   GMT. For `TZ=EST5`, one second after the Unix epoch is currently emitted as
   `Wed, 31 Dec 1969 19:00:01 GMT` rather than
   `Thu, 01 Jan 1970 00:00:01 GMT`.
2. `std::localtime()` exposes shared static calendar storage. Concurrent
   formatting can observe overwritten fields before `std::put_time()` has
   finished reading them.
3. A newly constructed stream inherits the process-wide C++ locale. HTTP
   weekday and month names must not silently change when an application
   selects a non-English global locale.

The first problem changes the instant represented by response and cookie
metadata. The latter two make server output dependent on unrelated process
state and concurrent work.

## Goals

- Keep the existing local-time formatting API and its local-time meaning.
- Make local and UTC calendar conversion safe for concurrent calls.
- Add explicit UTC formatting overloads.
- Define UTC output with the classic C locale so protocol names are stable.
- Emit response `Date` and cookie `Expires` from UTC calendar fields.
- Handle null formats and failed calendar conversion without undefined
  behavior.
- Cover the public API and both HTTP call sites with deterministic tests.

## Non-goals

- Parsing HTTP dates.
- Changing the timestamp storage type or its microsecond epoch.
- Changing server tracking/log timestamps from local time to UTC.
- Changing cookie precedence between `Max-Age` and `Expires`.
- Adding arbitrary timezone conversion or offset-aware formatting.
- Changing timestamp arithmetic operators in this round.

## Public API

`Timestamp` gains two additive overloads:

```cpp
std::string to_utc_format_str() const;
std::string to_utc_format_str(const char *fmt) const;
```

The no-argument overload uses the same default format as
`to_format_str()`:

```text
%Y-%m-%d %X
```

### Local formatting

`to_format_str()` and `to_format_str(fmt)` continue to:

- interpret the stored instant in the process local timezone;
- use the stream's existing locale behavior;
- return the formatted calendar text for valid inputs.

Their calendar conversion must no longer rely on shared static `tm` storage.

### UTC formatting

`to_utc_format_str()` and `to_utc_format_str(fmt)`:

- interpret the stored instant as UTC;
- use reentrant calendar conversion;
- format with `std::locale::classic()` so names and digits are stable;
- return the formatted calendar text for valid inputs.

### Invalid inputs and conversion failure

For both local and UTC formatting:

- `fmt == nullptr` returns an empty string;
- a calendar conversion failure returns an empty string;
- no formatter dereferences a null `tm` or format pointer.

An empty but non-null format string remains valid and produces an empty
string.

## HTTP behavior

Automatic response dates and cookie expiry dates use:

```text
%a, %d %b %Y %H:%M:%S GMT
```

with UTC calendar fields and the classic locale.

| Input instant | Process timezone | Required output |
| --- | --- | --- |
| 1970-01-01 00:00:01 UTC | UTC | Thu, 01 Jan 1970 00:00:01 GMT |
| 1970-01-01 00:00:01 UTC | EST5 | Thu, 01 Jan 1970 00:00:01 GMT |
| 1970-01-01 00:00:01 UTC | UTC-9 | Thu, 01 Jan 1970 00:00:01 GMT |

`HttpServer::track()` continues to call `to_format_str()` and therefore
continues to display process-local time.

## Implementation plan

1. Add a private translation-unit formatting helper that accepts a timezone
   mode, validates `fmt`, converts into caller-owned `std::tm` storage, and
   handles conversion failure.
2. Use the platform reentrant calendar APIs (`localtime_r`/`gmtime_r`, with
   their safe platform equivalents where required).
3. Preserve the existing local formatter's stream locale; imbue only the new
   UTC formatter with `std::locale::classic()`.
4. Add the UTC overload declarations and definitions.
5. Change `HttpServerTask` automatic `Date` generation and
   `HttpCookie::dump()` expiry generation to the UTC formatter.
6. Replace the print-only timestamp smoke test with assertions and add
   protocol regression coverage.

## Verification plan

### Timestamp unit tests

- A fixed instant differs between local and UTC formatting under `TZ=EST5`.
- UTC output remains the epoch-derived value under non-UTC timezones.
- Null formats return empty strings for both modes.
- The default local and UTC overloads are callable and non-empty for a valid
  instant.
- Concurrent calls using distinct instants repeatedly produce their expected
  local and UTC strings.

### Cookie unit tests

- Under `TZ=EST5`, `Expires` still contains
  `Thu, 01 Jan 1970 00:00:01 GMT`.
- Existing `Max-Age`, safety validation, and serialization tests remain
  unchanged.

### HTTP integration test

- Under `TZ=EST5`, an automatically generated `Date` matches a UTC timestamp
  within a small current-time window and cannot be the five-hour-shifted local
  representation.

### Tooling and regression

- Compile changed production and test sources with strict warning-as-error
  flags.
- Run focused timestamp, cookie, and HTTP header tests under ASan and UBSan.
- Run the complete registered CTest suite.

## Risks and mitigations

### Platform calendar APIs

The reentrant function signatures differ on Windows and POSIX systems. Keep
that variation inside one implementation helper and expose no platform types
in the public header.

### Global timezone changes in tests

Timezone mutation is process-global. Tests must save the original `TZ`, call
`tzset()` after each change, restore it with RAII, and perform no unrelated
work while the override is active. Each affected CTest executable runs in its
own process.

### Second-boundary integration flakiness

Do not compare the response date to a single sample. Generate an allowed UTC
window around the callback's current second; the timezone bug differs by
hours, while normal scheduling differs by seconds.

## Acceptance criteria

- The fixed non-UTC reproduction emits the correct UTC HTTP date.
- `Timestamp` local formatting remains local and becomes reentrant.
- The new UTC API is timezone-independent and classic-locale formatted.
- Response `Date` and cookie `Expires` use the UTC API.
- Null format pointers and failed time conversion return empty output.
- Strict builds, focused sanitizer checks, and the full test suite pass.
