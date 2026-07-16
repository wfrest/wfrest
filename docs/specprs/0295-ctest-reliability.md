# Reliable CTest registration

Status: Proposed

Issue: [#295](https://github.com/wfrest/wfrest/issues/295)

Integration branch: `test`

## Motivation

`make check` must answer one question reliably: does every automated test that
the target builds pass? The current CMake model cannot provide that guarantee.
It registers memory-check commands even when Valgrind is unavailable, omits
four finite smoke executables from CTest, gives asynchronous tests no upper
time bound, and allows fixed-port server tests to collide when CTest runs in
parallel.

The baseline at commit `496f11a` demonstrates the gap:

- CTest registers 44 entries without Valgrind: 22 functional entries and 22
  unusable `*-memory-check` entries.
- Every unusable entry resolves to
  `CMAKE_MEMORYCHECK_COMMAND-NOTFOUND`.
- `router_test`, `multi_part_test`, `StringPiece_feature`, and
  `RouteTableNode_test` are compiled by `check` but absent from `ctest -N`.
- All server suites use port 8888; `proxy_test` additionally uses port 8887.

## Goals

1. Keep the default functional suite valid on hosts both with and without
   Valgrind.
2. Ensure every finite automated executable built by `check` is registered in
   CTest.
3. Bound test duration so a stalled async callback fails instead of blocking
   CI indefinitely.
4. Make `ctest -j` safe for suites that share fixed ports.
5. Preserve the existing source compatibility and test program behavior.

## Non-goals

- Rewriting the tests or production code.
- Making the interactive `multi_verb_test` automated; it waits on `getchar()`
  by design.
- Replacing fixed ports with dynamic allocation in this change.
- Selecting a machine-wide GTest or C++ runtime when a developer environment
  exposes conflicting third-party installations.

## Proposed design

### Memory checks are capability-gated

Continue discovering Valgrind with `find_program`. Add memory-check entries
only inside an `if(CMAKE_MEMORYCHECK_COMMAND)` block. A host without Valgrind
therefore runs the same functional tests and has no invalid CTest commands. A
host with Valgrind retains the existing memory-check coverage and options.

### Automated and manual executables are explicit

Split the current miscellaneous list into:

- `SMOKE_TEST_LIST`: the four finite executables, all registered with CTest;
- `MANUAL_TEST_LIST`: `multi_verb_test`, which remains buildable through the
  `check` target but is not registered because it intentionally blocks for
  terminal input.

This classification makes omissions visible during review and avoids turning
an interactive example into a CI hang.

### Test execution is bounded and concurrency-safe

Set a 30-second timeout on unit and smoke entries and a 60-second timeout on
HTTP server entries. Apply the same server timeout to their memory-check
variants only if later measurement shows 60 seconds is sufficient; otherwise
use a documented larger bound for memory checks.

Set `RESOURCE_LOCK wfrest_test_server_ports` on all server functional and
memory-check entries. CTest may still parallelize independent unit and smoke
tests, but only one suite that owns ports 8887/8888 may run at a time.

### Check output remains diagnostic

Invoke CTest with `--output-on-failure` directly from the `check` target rather
than relying on a make-variable side channel. This keeps direct
`cmake --build ... --target check` and GNUmakefile entry points consistent.

## Validation plan

1. Configure in a fresh test build directory with
   `CMAKE_MEMORYCHECK_COMMAND` forced unavailable; verify `ctest -N` contains
   exactly 26 functional tests and no `memory-check` entries.
2. Build `check` and run all 26 entries with the system GTest toolchain.
3. Inspect generated CTest metadata to confirm all server entries share the
   same `RESOURCE_LOCK` and every test has a timeout.
4. When Valgrind is installed, reconfigure with its executable and verify 22
   additional memory-check entries are present and executable.
5. Run `ctest -j 8` to verify shared-port suites remain serialized.

## Risks and mitigations

- **Smoke programs have weak assertions.** Running them still detects crashes
  and sanitizer failures. Assertion improvements belong in follow-up issues.
- **Timeouts can be too strict on slow CI.** The proposed bounds are well above
  observed normal runtimes and can be raised from measured evidence without
  removing the safety property.
- **Valgrind runs are slower.** Memory-check entries receive a separate, larger
  timeout so capability gating does not trade false registration failures for
  timeout failures.

## Rollback

The change only affects test registration. Reverting the implementation
restores the previous CMake behavior without changing library ABI, production
artifacts, or test source semantics.
