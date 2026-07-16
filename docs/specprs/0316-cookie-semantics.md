# Standard request cookies and safe Set-Cookie output

Issue: #316

## Motivation

wfrest currently splits `Cookie` on commas, splits every equals sign, reads
only the first header field, and treats an empty value as an invalid cookie.
The response serializer also omits `Max-Age=0` and copies unvalidated strings
into `Set-Cookie`.

These behaviors break ordinary session parsing and deletion, and allow control
or delimiter bytes supplied to the response API to cross an HTTP header
boundary.

## Goals

1. Parse standard semicolon-delimited cookie pairs in one pass.
2. Preserve the complete value after the first equals sign.
3. Merge all request `Cookie` fields once with deterministic first-wins rules.
4. Support empty values and explicit non-positive `Max-Age` values.
5. Reject invalid response cookie data before header emission.
6. Preserve valid existing attributes and `SameSite=None` secure behavior.

## Non-goals

- Percent-decoding or URL-decoding cookie values.
- Cookie storage, expiry evaluation, or browser policy enforcement.
- Public-suffix, DNS, or domain-ownership validation.
- Supporting obsolete comma-separated request cookie pairs.

## Request parsing

`HttpCookie::split` scans each segment ending at `;`. Within a non-empty
segment, only the first `=` separates the name from the value. HTTP optional
whitespace (`SP` and `HTAB`) is removed from both ends of each side.

A pair is inserted only when:

- the equals sign is present;
- the trimmed name is non-empty and every byte is an HTTP token character;
- the trimmed value is empty, a valid unquoted cookie value, or a quoted value
  whose contents are valid cookie octets;
- a key with the same exact case has not already been inserted.

Cookie names remain case-sensitive. The allowed token punctuation is:

```text
! # $ % & ' * + - . ^ _ ` | ~
```

Cookie value bytes follow the cookie-octet ranges: visible ASCII excluding
space, double quote, comma, semicolon, and backslash. A surrounding pair of
double quotes is removed; a lone or embedded quote is invalid. No escape or
percent decoding occurs.

Malformed pairs are skipped without affecting later segments. Examples:

| Input | Result |
| --- | --- |
| `user=alice; role=admin` | `user=alice`, `role=admin` |
| `token=a=b=c` | `token=a=b=c` |
| `flag=` | `flag` with empty value |
| `quoted="abc"` | `quoted=abc` |
| `missing` | skipped |
| ` =value` | skipped |
| `a=first; a=second` | `a=first` |

## Multiple request headers and caching

`HttpReq::cookies()` parses every stored `Cookie` header value in arrival
order. Results are merged with `map::emplace`, so the first exact name across
all fields wins. A dedicated `cookies_parsed_` flag is set even when no valid
pair exists; repeated `cookies()` or `cookie()` calls never rescan the headers.

The flag participates in move construction and move assignment with the cookie
map.

## Response validation

An `HttpCookie` is valid when:

- its name is a non-empty HTTP token;
- its value is empty or contains only cookie octets;
- Domain and Path contain only ASCII bytes from SP through `~`, excluding
  semicolon; CR, LF, other controls, DEL, and non-ASCII bytes are rejected.

An empty value is valid and `operator bool` reflects this validation. `dump()`
returns an empty string for invalid state. `HttpServerTask` skips a cookie whose
dump is empty, so invalid input cannot create an empty or injected
`Set-Cookie` field.

This validation intentionally does not rewrite or encode input. Applications
must provide an already valid cookie value.

## Max-Age presence

The object stores `has_max_age_` separately from signed `max_age_`.
`set_max_age(value)` sets both. Whenever present, dump emits the exact signed
decimal value, including zero and negatives, and suppresses `Expires` as before.
Without a call to `set_max_age`, the attribute remains absent.

Examples:

```text
session=; Max-Age=0; Path=/; HttpOnly
session=; Max-Age=-1
```

## Serialization

Valid output retains the existing attribute order:

1. name/value;
2. Max-Age, otherwise Expires;
3. Domain;
4. Path;
5. Secure;
6. HttpOnly;
7. SameSite;
8. implicit Secure when SameSite is None and Secure was not already set.

No trailing semicolon or space is emitted.

## Compatibility

Valid single request cookies and existing Set-Cookie attributes remain
compatible. Standard semicolon input, multiple header fields, embedded equals,
quoted values, empty values, and deletion ages become supported.

Comma-separated pairs are intentionally no longer recognized as multiple
cookies. Invalid response strings that were previously emitted verbatim now
produce no header.

## Test plan

### Unit tests

- semicolon pairs, OWS, embedded equals, empty and quoted values;
- missing equals, empty/invalid names, invalid values, and malformed quotes;
- duplicate first-wins behavior;
- empty-value truthiness and dump;
- zero/negative/positive Max-Age precedence over Expires;
- every disallowed value byte class and Domain/Path header injection;
- valid attributes and SameSite=None compatibility.

### HTTP integration

- send multiple `Cookie` header fields with an overlapping name;
- verify the handler sees all valid unique pairs and the first duplicate;
- call request cookie access twice and verify stable results;
- add valid deletion and invalid response cookies;
- verify exactly one safe `Set-Cookie` header is emitted.

### Validation

- focused parser/serializer tests under ASan and UBSan;
- focused Cookie unit and HTTP integration CTest;
- full CTest suite;
- strict C++11 warning compile for touched code;
- `git diff --check`.
