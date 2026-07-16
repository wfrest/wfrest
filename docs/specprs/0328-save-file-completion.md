# Exact and complete asynchronous file saves

Issue: #328

## Motivation

`HttpResp::Save()` currently uses Workflow's path-based pwrite task. That task
opens the destination with `O_WRONLY | O_CREAT` and writes at offset zero. It
does not truncate an existing destination.

The wfrest completion callback accepts every non-negative return value. POSIX
allows a successful `pwrite` call to transfer fewer bytes than requested, so a
partial destination can produce the normal success notification.

On `test` at `e1b017c`, syscall probes matching the current path task show:

- writing `new` over `new-old-tail` leaves `new-old-tail`;
- with a five-byte `RLIMIT_FSIZE`, a ten-byte pwrite returns five rather than a
  negative error.

An empty save also leaves an existing destination unchanged. The path task
closes its internal fd before calling wfrest and ignores close failure, so the
application cannot include close completion in its success decision.

## Goals

1. Make each successful save an exact replacement of the previous file bytes.
2. Define success as full byte-count completion plus successful close.
3. Never emit a success notification for partial or failed writes.
4. Own and release the destination fd exactly once on every completion path.
5. Preserve every public `Save` overload and the completion callback view.
6. Keep asynchronous error completion for destination-open failures.

## Non-goals

- Atomic replacement through a temporary file and rename.
- Crash durability through `fsync`, `fdatasync`, or directory sync.
- Serializing concurrent saves to the same path.
- Changing new-file mode, umask behavior, symlink following, or ownership.
- Retrying a short write.
- Rolling back a partially written file after an asynchronous failure.

## Destination opening and replacement

wfrest opens the destination before constructing the Workflow task with:

```text
O_WRONLY | O_CREAT | O_TRUNC, mode 0644
```

For an existing file, `O_TRUNC` removes all old bytes before the asynchronous
write. For a new file, mode remains `0644` subject to the process umask, matching
the previous Workflow path task.

Opening an empty save still truncates the destination. Its zero-byte pwrite must
return zero, and close must succeed, for the save to be successful.

If open fails, wfrest retains fd `-1` and still constructs the fd-based pwrite
task. The task therefore completes through the same asynchronous callback with
a write error. This preserves the existing completion timing and ensures a
provided `FileIOArgsFunc` is invoked once for open failures.

## Save context ownership

`SaveFileContext` owns:

- the copied or moved content buffer;
- the optional notification string;
- the optional `FileIOArgsFunc`;
- the opened destination fd, or `-1`;
- the expected byte count.

The pwrite task receives the context buffer, expected count, offset zero, and
the context fd. `user_data` is installed before the task is added to the server
series.

The normal pwrite callback closes an owned non-negative fd once and immediately
sets both the context fd and `FileIOArgs::fd` to `-1`. It never retries `close`
after `EINTR`, because the fd state is unspecified and retrying can close a
reused descriptor.

The server-task cleanup callback is a fallback: if the pwrite callback did not
consume the fd, cleanup closes it once before deleting the context. This covers
abnormal series termination without double-close.

## Completion contract

The pwrite callback preserves the existing ordering for user completion hooks:

1. obtain the task result and args;
2. close and release the owned fd;
3. set `args->fd` to `-1`;
4. invoke `FileIOArgsFunc`, when present;
5. decide response success or failure;
6. append the notification only on success.

A save is successful only when all conditions hold:

1. task state is `WFT_STATE_SUCCESS`;
2. return value is non-negative;
3. return value equals the context expected count without signed narrowing;
4. the destination fd existed and `close` returned zero.

Any other result calls `HttpResp::Error(StatusFileWriteError)`. A failure never
appends `notify_msg`. The destination may be empty or contain a partial prefix,
which is why atomic rollback is explicitly outside this change.

`FileIOArgsFunc` is observational: it is invoked once for task completion on
both success and failure. As with the previous path-based Workflow task, it
observes `args->fd == -1`; the buffer, count, and offset remain available for
the duration of the callback.

## Overload behavior

Both private lvalue and rvalue `save_file` implementations populate the same
context and delegate to one enqueue helper. This prevents the ownership,
truncation, expected-count, and cleanup behavior from diverging between public
overloads.

The lvalue overload copies content. The rvalue overload moves content. The
context retains both buffers until the server task completes, so pwrite and
no-copy notification pointers stay valid.

## Compatibility

Public signatures and normal successful responses do not change. Saving a new
file, using const or moved content, returning a custom success notification,
and observing pwrite args continue to work.

Intentional behavior changes are limited to invalid success cases and old-tail
retention:

- shorter and empty saves now truncate old content;
- short and overlong task results now produce the file-write error;
- close failure now produces the file-write error;
- success text is suppressed for every incomplete save.

## Implementation plan

1. Add the POSIX open/close includes to `HttpFile.cc`.
2. Extend `SaveFileContext` with fd and expected count.
3. Add one helper that opens with truncation, constructs the fd-based pwrite
   task, installs cleanup, and queues it.
4. Refactor both private save overloads to populate a context and call the
   helper.
5. Make the callback release the fd, preserve the user callback view, compare
   exact byte counts, include close success, and gate notification output.

## Test plan

### Replacement and overloads

- replace an existing longer file with shorter const-string content;
- replace an existing file with empty content;
- save moved string content to a new file;
- verify new file contents and old-tail removal after server completion.

### Completion behavior

- success notification is returned only after a complete write;
- a nonexistent parent directory produces `StatusFileWriteError` and invokes
  the user completion callback with fd `-1`;
- `/dev/full` produces an error and no success notification when available;
- a temporary five-byte soft `RLIMIT_FSIZE` makes a ten-byte save return the
  file-write error, leaves a five-byte partial file, and suppresses success
  text;
- restore the old resource limit and `SIGXFSZ` handler after the test.

### Validation

- strict C++11 compile with `-Wall -Wextra -Wpedantic -Werror`;
- focused file-save integration tests under ASan/UBSan/LSan where compatible;
- complete CTest suite;
- fd-count or sanitizer leak verification;
- `git diff --check`.
