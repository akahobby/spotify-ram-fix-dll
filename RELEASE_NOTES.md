# v1.0 release candidate

- Preserves the v0.9.3 cross-process coordinator and self-trim fallback.
- Adds a single-download Windows release artifact workflow.
- Adds PowerShell install/uninstall with backup and hash checks.
- Adds usage, limitations, rollback and verification documentation.
- No MMCSS, raw input or thread-priority hooks.

**Known limitation:** Not every Spotify subprocess loads the local proxy; cross-process trim attempts can fail from Spotify sandbox permissions. Working-set trimming reduces resident memory temporarily, not necessarily private commit.

**Testing:** Prior v0.9.3 version was reported working by the user; this packaged v1.0 source has not been compiled/tested on Windows in this environment.
