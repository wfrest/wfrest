# Saturating Timestamp arithmetic and exact microsecond text

Issue: #355

## Summary

Make `Timestamp` text preserve six-digit microsecond precision, make time-point
addition/subtraction saturate within its unsigned epoch domain, and make
timestamp differences signed. Floating-second operations must handle negative,
oversized, infinite, and NaN values without undefined conversions.

## Why

`Timestamp` stores a `uint64_t` microsecond count. Current operators apply
ordinary unsigned arithmetic, so crossing either endpoint wraps to the other
side of the domain. The double overload first casts `seconds * 1,000,000` to
`uint64_t`; negative and sufficiently large values are outside that conversion
domain and trigger undefined behavior.

Current examples:

```text
1000001us.to_str()     -> "1.1"
1s - 2s               -> +18446744073709.55... seconds
1us - 2us             -> UINT64_MAX
UINT64_MAX + 1us      -> 0
1s + (-2s)            -> sanitizer failure / far future
```

## Goals

- Emit unambiguous fixed-six-digit fractional seconds.
- Preserve all in-range arithmetic results.
- Saturate time points at epoch zero and `UINT64_MAX`.
- Give negative floating deltas their mathematical direction.
- Avoid every out-of-range floating-to-unsigned conversion.
- Return negative timestamp differences when the left operand is earlier.
- Cover all numeric boundaries with deterministic tests.

## Non-goals

- Changing the unsigned storage type.
- Representing a time point before the Unix epoch.
- Adding checked/optional arithmetic APIs.
- Changing constructor, comparison, validity, calendar formatting, or UTC
  behavior.
- Changing the integer operator's unit from microseconds.
- Rounding fractional microseconds instead of truncating toward zero.

## `to_str()` contract

Return:

```text
<whole seconds>.<six decimal microsecond digits>
```

Examples:

| Stored microseconds | Output |
| ---: | --- |
| 0 | `0.000000` |
| 1 | `0.000001` |
| 1,000,001 | `1.000001` |
| 1,100,000 | `1.100000` |

The whole-seconds field has no unnecessary leading zeros. The fractional
field always has exactly six digits.

## Integer arithmetic

The existing signatures remain, with accurate parameter names:

```cpp
Timestamp operator+(Timestamp lhs, uint64_t microseconds);
Timestamp operator-(Timestamp lhs, uint64_t microseconds);
```

### Addition

If `microseconds <= UINT64_MAX - lhs`, return the exact sum. Otherwise return
`Timestamp(UINT64_MAX)`.

### Subtraction

If `microseconds <= lhs`, return the exact difference. Otherwise return
`Timestamp(0)`.

No unsigned operation is evaluated before its range condition.

## Floating-second arithmetic

The existing signatures remain:

```cpp
Timestamp operator+(Timestamp lhs, double seconds);
Timestamp operator-(Timestamp lhs, double seconds);
```

### Finite positive delta

Convert the magnitude to microseconds in `long double` precision. Truncate the
fractional microsecond toward zero. Add/subtract through the same saturation
rules as integer arithmetic.

### Finite negative delta

- `lhs + negative` subtracts the magnitude;
- `lhs - negative` adds the magnitude.

This is directional, not a cast through `uint64_t`.

### Zero and NaN

Positive zero, negative zero, and NaN return `lhs` unchanged.

### Infinity and oversized finite magnitude

| Operation | Positive magnitude | Negative magnitude |
| --- | --- | --- |
| `lhs + seconds` | saturate maximum | saturate zero |
| `lhs - seconds` | saturate zero | saturate maximum |

Compare the long-double microsecond magnitude against available room before
any conversion to `uint64_t`.

## Timestamp difference

```cpp
double operator-(Timestamp lhs, Timestamp rhs);
```

- if `lhs >= rhs`, calculate `(lhs_us - rhs_us) / 1,000,000.0`;
- if `lhs < rhs`, calculate `-(rhs_us - lhs_us) / 1,000,000.0`.

The subtraction inside each branch is ordered and cannot underflow. The result
is antisymmetric within normal floating precision:

```text
(a - b) == -(b - a)
```

## Implementation plan

1. Format `to_str()` with a stream, `setw(6)`, and zero fill.
2. Add `<cmath>` and `<limits>` to the inline operator header dependencies.
3. Replace integer operators with pre-checked saturating branches.
4. Implement the double addition path by sign/magnitude classification in
   long double, then let double subtraction reverse the sign and delegate.
5. Handle NaN before sign comparisons and conversion.
6. Branch timestamp difference by ordering before subtracting.
7. Expand `TimeStamp_unittest` with serialization and numeric boundary tests.

## Verification plan

### Text

- epoch, single microsecond, mixed seconds/microseconds, and trailing zeros.

### Integer operators

- exact interior addition/subtraction;
- exact endpoint result;
- one-unit overflow/underflow saturation;
- large delta from both invalid and maximum timestamps.

### Double operators

- positive and negative whole seconds;
- sub-microsecond and fractional-microsecond truncation;
- positive/negative zero;
- positive/negative infinity;
- positive/negative oversized finite values;
- NaN unchanged;
- no `float-cast-overflow` sanitizer report.

### Difference

- positive, zero, and negative differences;
- antisymmetry;
- full-domain endpoint difference remains finite with the correct sign.

### Regression

- Existing local/UTC formatting, locale, null-format, and concurrency tests.
- Strict C++11/C++14 warning-as-error builds.
- ASan, UBSan, and explicit float-cast-overflow sanitizer run.
- Full registered CTest suite.

## Risks

### Invalid-case behavior changes

Code that accidentally relied on unsigned wraparound will observe saturation.
Wraparound time points are not meaningful epoch values; saturation makes the
domain boundary explicit and deterministic.

### Floating precision

Large doubles cannot express individual microseconds. Long double is used for
range classification and scaling, but the input's original double precision
cannot be recovered. In-range conversion continues to truncate, matching the
old positive-delta behavior.

## Acceptance criteria

- `to_str()` always has six fractional digits.
- All time-point arithmetic stays within the unsigned domain without wrap.
- Negative/NaN/infinite seconds never trigger invalid numeric conversion.
- Timestamp differences have the correct sign.
- Strict builds, focused sanitizers, and 37-test regression suite pass.
