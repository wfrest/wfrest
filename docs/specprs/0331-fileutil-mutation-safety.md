# Truthful and bounded FileUtil mutations

Issue: #331

## Motivation

The current `FileUtil` mutation helpers often report only the final operation,
not whether the requested work completed. Intermediate mkdir, recursive remove,
stream write, and close failures are ignored.

Recursive removal also calls `stat` before recursion. Because `stat` follows a
directory symlink, a link inside the requested tree can redirect deletion into
an unrelated directory.

Controlled probes on `test` at `aefc115` demonstrate that:

- removing a tree with a directory symlink deletes a file in the symlink target
  outside the tree;
- `create_directories("blocker/")` returns true when `blocker` is a regular
  file;
- `create_file_with_size("/dev/full", 4096)` returns true after write failure.

The return values must be trustworthy, and recursive deletion must stay bound
to directory objects opened from the requested root.

## Goals

1. Check every component of recursive directory creation.
2. Never traverse a symlink during recursive deletion.
3. Refuse mount/device boundary traversal.
4. Propagate every filesystem enumeration and mutation failure.
5. Report generated-file write and close failures.
6. Make file-size queries null-safe and range-safe.
7. Reject accidental deletion of filesystem root or the current directory.

## Non-goals

- Rolling back directories already created before a later component fails.
- Rolling back a partially generated file after write failure.
- Cryptographically secure generated content.
- Deleting across filesystem or mount boundaries.
- Preserving a tree against an attacker moving an already-open directory after
  it was reached from the root.
- Changing public signatures or adding rich error objects.

## File size contract

`FileUtil::size(path, output)` first rejects a null output pointer with
`StatusFileReadError`. For a non-null output, it initializes `*output` to zero
before calling `stat`.

The result mapping is:

- `ENOENT` or `ENOTDIR`: `StatusNotFound`;
- another `stat` failure: `StatusFileReadError`;
- a non-regular filesystem object: `StatusFileReadError`;
- negative `st_size` or a value above `size_t` maximum:
  `StatusFileReadError`;
- otherwise: store the size and return `StatusOK`.

Symlink behavior remains the behavior of `stat`: a symlink to a regular file is
measured as that file. Broken symlinks are not found.

## Recursive directory creation

An empty path is invalid and returns false. A path consisting only of `/`
separators identifies root and returns true after verifying root is a directory.

The input is scanned left to right without erasing or normalizing its semantic
components. Repeated separators and a final separator are ignored as separators,
while `.` and `..` remain ordinary filesystem components.

For every accumulated component:

1. call `mkdir(component, 0755)`;
2. continue on success;
3. on `EEXIST`, call `stat` and continue only when the existing object resolves
   to a directory;
4. return false for every other error or object type.

Thus the function returns true only when the complete requested path resolves
to a directory. New-directory mode remains `0755` subject to umask.

## Descriptor-based recursive removal

Root traversal starts from an opened `/` descriptor for absolute input or an
opened `.` descriptor for relative input. Each non-empty component other than
`.` is then opened relative to the previous descriptor with:

```text
O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC
```

This component walk applies `O_NOFOLLOW` to every user-supplied component, not
only the final name. A root path such as `directory-link/.` therefore cannot
bypass symlink rejection. `..` retains its normal filesystem meaning but is
also opened relative to the current descriptor without following a symlink.

Any component-open or prior-descriptor close failure returns false. `fstat`
records the final root device and directory identity. The opened root is
compared with separately component-opened `/` and `.` descriptors; matching
filesystem root or current directory returns false before enumerating any
entry.

The recursive walker transfers each owned fd to `fdopendir`. For each entry
other than `.` and `..`, it calls:

```text
fstatat(parent_fd, name, ..., AT_SYMLINK_NOFOLLOW)
```

Entries are handled as follows:

- a directory on the root device is opened relative to its parent with
  `O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC`, recursively emptied, then removed
  with `unlinkat(..., AT_REMOVEDIR)`;
- a directory on another device makes the operation fail without opening or
  removing that entry;
- a symlink or any non-directory entry is removed with `unlinkat(..., 0)` and
  is never opened as a traversal target.

The first `readdir`, `fstatat`, `openat`, recursive removal, or `unlinkat`
failure stops the walk. `readdir` terminal errno and `closedir` are also
checked. Every descriptor is consumed by `fdopendir`, explicitly closed after
setup failure, or closed by a small identity-check helper.

Only after the root contents are completely removed and its stream is closed
does `remove_directory` call `rmdir`. The already-verified path is normalized
lexically for that final call by collapsing repeated separators and `.` and by
resolving `..` components without filesystem lookup. This lets ordinary forms
such as `target/./` remove the intended root instead of failing after it was
emptied. It returns true only if the final `rmdir` succeeds.

## Generated file completion

`create_file_with_size` retains the existing binary truncating stream and
printable random ASCII generation. Each chunk is written and stream state is
checked before decrementing the remaining count.

After all requested bytes are written, the stream is explicitly closed and its
failure state checked. The function returns true only when every write and the
close complete. A successful file therefore contains exactly the requested
number of bytes, each in inclusive ASCII range 32 through 126.

Zero size remains valid: an existing file is truncated, close is checked, and
the empty file is reported as success.

## Failure and partial-state semantics

These bool APIs report completion but do not provide rollback:

- mkdir failure can leave already-created parent components;
- remove failure can leave an already-partially-emptied tree;
- generated-file failure can leave a truncated or partial file.

The important invariant is that false never authorizes a caller to assume full
completion, and true means the postcondition is verified by all relevant
operations.

## Compatibility

Successful normal uses remain compatible: nested relative or absolute mkdir,
recursive removal of an ordinary tree, regular-file sizing, and generated test
files retain their public forms.

Intentional changes affect unsafe or false-success cases:

- empty mkdir input now fails;
- an `EEXIST` regular file no longer counts as a directory;
- root symlinks are not accepted as remove roots;
- child symlinks are unlinked rather than traversed;
- mounted child directories stop removal;
- ignored write, close, enumeration, and child-removal failures now return
  false;
- size errors are classified instead of crashing or narrowing.

## Implementation plan

1. Add local helpers for directory verification and component-wise mkdir.
2. Add nofollow root-component, fd identity, and recursive descriptor-walk
   helpers.
3. Replace path-based recursive removal with `openat`/`unlinkat` traversal.
4. Check generated stream state after every write and close.
5. Harden the size query before `stat` conversion.
6. Expand focused tests without changing the public header.

## Test plan

### Size and mkdir

- regular, missing, directory, symlinked regular file, and null-output size;
- nested relative and absolute paths;
- repeated and trailing separators;
- existing directories and regular-file conflicts;
- empty input and root.

### Recursive removal

- nested directories and files;
- child symlink to an external directory, preserving all external contents;
- root symlink rejection;
- missing path and regular-file root rejection;
- root/current-directory guards;
- failure propagation for an entry that cannot be removed where the runtime
  permits a deterministic setup.

### Generated files and validation

- zero-size truncation;
- exact multi-chunk size and printable byte range;
- missing parent and `/dev/full` failures;
- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- focused ASan/UBSan/LSan tests;
- complete CTest suite;
- `git diff --check`.
