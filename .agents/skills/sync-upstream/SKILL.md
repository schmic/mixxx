---
name: sync-upstream
description: Explicitly invoke to fetch official Mixxx upstream and rebase MixxxOS main, resolve integration conflicts, validate, and clean up the temporary backup. Not for general Git questions or unsolicited synchronization.
---

# Sync Mixxx upstream

Use only when the user explicitly invokes `$sync-upstream` or explicitly asks
to execute this synchronization workflow. Merely discussing, installing, or
editing this skill does not authorize a fetch or rebase.

The shared workflow is stored in `.opencode/commands/sync-upstream.md`, relative
to this repository's root. Read that file in full before taking any action.
Ignore its OpenCode-specific YAML frontmatter; follow the Markdown workflow
body as instructions for this task. Do not invoke OpenCode or require its CLI.
If the workflow file is missing or unreadable, stop and report the problem;
do not improvise a replacement or fetch instructions from another repository.

Honor all applicable AGENTS.md instructions, Codex mode restrictions, sandbox
rules, and tool permissions. In Plan/read-only mode, inspect only and ask the
user to enable a write-capable mode before fetching or rebasing. A workflow's
instruction not to request additional confirmation never bypasses required
tool approval or sandbox restrictions.

The shared workflow defines the safety checks, temporary backup, conflict
resolution, validation, and guarded cleanup. Explicit execution authorizes
those operations without an extra pre-rebase confirmation. It never authorizes
pushing, force-pushing, standalone commits, or publishing PRs/issues. Preserve
existing work and retain the backup on blockers as specified in that file.

Keep the shared workflow as the single source of truth. When changing sync
behavior, update that file rather than duplicating its steps here. Preserve
the existing OpenCode `/sync-upstream` command.
