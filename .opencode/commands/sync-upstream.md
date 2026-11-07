---
description: Fetch upstream main and safely rebase MixxxOS main
subagent: false
---

Synchronize this MixxxOS fork by replaying its local `main` commits on top of
the official Mixxx `upstream/main`. Do not rebase upstream onto MixxxOS.
Work in this session; do not delegate. Follow all applicable AGENTS.md rules.
Invoking this command explicitly authorizes fetching and rebasing local main
as described below, including rewriting its previously published commits,
resolving rebase conflicts, continuing the rebase, fixing integration issues,
and running the validation below. It also authorizes deleting this invocation's
temporary backup branch after successful validation.
Do not request an additional pre-rebase confirmation. This does not authorize
pushing rewritten history or bypassing safety checks.

## Safety and preflight

1. In Plan mode, perform read-only inspection only and ask the user to switch
   to a build-capable agent before fetching or rebasing. Do not bypass mode
   restrictions or tool permissions.
2. Locate the repository root and inspect:

   ```sh
   git status --short --branch
   git remote -v
   git branch -vv
   git rev-parse HEAD
   git branch --show-current
   git status --porcelain=v1 --untracked-files=all
   ```

3. Require the current branch to be `main`, with no staged, unstaged, or
   untracked files. Stop if dirty, detached, or on another branch. Never
   automatically stash, stage existing work, commit, discard files, or switch branches.
   The human must review and preserve their work first.
4. Check for an active rebase, merge, cherry-pick, revert, or bisect using Git
   status and Git's resolved state paths (`git rev-parse --git-path ...`).
   Stop if any operation is in progress; never abort it automatically.
5. Verify `upstream` identifies official `mixxxdj/mixxx` and `origin` identifies
   the user's MixxxOS fork. Do not change remotes. If ambiguous, stop and ask.
   Read CONTRIBUTING.md and current AGENTS.md before proceeding.

## Fetch and review

6. Fetch fresh main refs without pruning or fetching tags:

   ```sh
   git -c fetch.prune=false -c fetch.pruneTags=false fetch --no-tags upstream refs/heads/main:refs/remotes/upstream/main
   git -c fetch.prune=false -c fetch.pruneTags=false fetch --no-tags origin refs/heads/main:refs/remotes/origin/main
   ```

   Stop on any failure, including non-fast-forward remote-ref updates. Do not
   force a ref update or use stale refs as proof of publication status.
7. Record full SHAs for `main`, `upstream/main`, and `origin/main`. Find the
   merge base between local and upstream main; stop if no common base exists.
   Inspect incoming upstream commits/diff and local-only commits. Compare
   local AGENTS.md with `git show upstream/main:AGENTS.md`; preserve upstream
   policy and MixxxOS instructions. Do not silently edit files during review.
8. If origin has commits absent from local main, stop: this command must not
   drop or silently integrate fork-side work. If local main contains merge
   commits that would be replayed, stop and ask for a deliberate merge-history
   strategy rather than flattening them with a default rebase.
9. If upstream main is already an ancestor of local main, report already
   synchronized; do not rewrite history. Otherwise, report proposed old/new
   base SHAs, incoming commits, local commits to replay, and conflicts likely
   to affect MixxxOS or `res/skins/MixxxOSQML/`.
10. Determine whether any commits to be replayed are reachable from freshly
    fetched origin main or other known remote-tracking branches. Warn that
    rebasing published commits rewrites history, and that refs cannot prove
    nobody else relies on them. Report this without a confirmation gate;
    invocation already authorizes the local rebase. Coordination with other
    contributors and publication remain the human maintainer's responsibility.

## Rebase

11. Recheck branch, clean working tree, operation state, and all three SHAs
    immediately before rebasing. If anything changed during review, stop and
    report it rather than acting on stale state.
    Create a uniquely named local backup branch pointing to the recorded
    pre-rebase main SHA, without switching branches or overwriting any ref.
    Report and retain the exact backup ref and SHA for validation and cleanup.
    Never reuse an existing backup branch or clean up another invocation's ref.
12. Rebase current main onto the recorded, pinned upstream SHA. Disable
    automatic stashing, automatic conflict reuse, and updating other branches:

    ```sh
    git -c rerere.enabled=false -c rebase.updateRefs=false rebase --no-autostash <pinned-upstream-SHA>
    ```

    Replace the placeholder only with the verified full SHA, not arbitrary
    user arguments. Never run reset, clean, automatic rebase skip, force-push,
    or a destructive checkout. On conflicts, inspect the upstream change,
    the local commit being replayed, and both sides' intended behavior. Resolve
    conflicts narrowly while preserving upstream fixes and MixxxOS features,
    licence/attribution, project policies, and real-time audio requirements.
    Rebase-side "ours" and "theirs" do not mean the same thing as in a normal
    merge; never choose a whole side blindly.

    Stage only resolved files for this rebase, then use
    `git -c rerere.enabled=false -c rebase.updateRefs=false -c core.editor=true rebase --continue` to preserve existing commit
    messages without an interactive editor. Repeat until complete. Do not run
    a separate `git commit`. If a commit becomes empty, verify and report
    whether its intent is already present; do not silently skip it. If intent
    is ambiguous, required behavior cannot be preserved, or safe resolution
    is blocked, stop with the backup intact and explain the decision needed.
    Never abort automatically or claim every possible conflict is fixable.
13. On success, verify current branch is still main, working tree is clean,
    and the pinned upstream SHA is an ancestor. Review old/new local patches
    with `git range-diff` where applicable. Report old/new main and upstream
    SHAs, replayed commits, conflict resolutions, and any unexpected changes.

## Integration fixes and validation

14. Inspect this checkout's current build instructions and available build
    configuration. Configure with QML and tests enabled, build, and run the
    relevant tests. Typical commands, subject to verifying current options:

    ```sh
    cmake -S . -B build -DQML=ON -DBUILD_TESTING=ON
    cmake --build build --parallel 12
    ctest --test-dir build --output-on-failure --parallel 4
    ```

    Use disposable profiles/libraries, including isolated HOME/XDG directories
    where needed. Run the skin's documented static lint. Smoke-test configured
    MixxxOSQML with `--developer`, never `--new-ui`, at 1280 x 800 logical pixels
    when a safe runtime environment is available. Check discovery/install
    behavior and standard-skin fallback if affected by incoming changes.
15. Fix issues introduced or exposed by the upstream integration, not unrelated
    pre-existing problems. Update compatibility code and focused regression
    tests as needed, preserving user choices and existing functionality. Do
    not disable tests, weaken assertions, delete features, mass-format files,
    install packages globally, or change system configuration to make checks
    pass. Rerun affected checks and report exact results and limitations.
    Missing dependencies, ambiguous design decisions, unavailable safe runtime
    testing, or unsolved failures are blockers: retain the backup and report.
16. Post-rebase integration fixes must remain uncommitted for human review;
    report their files and a proposed commit boundary. Do not create additional
    commits or amend/rewrite completed commits to hide those fixes. If such
    fixes leave the working tree dirty, keep the backup until the human records
    the fixes and validation is complete.

## Backup cleanup and handoff

17. Delete only this invocation's backup after the rebase is complete, required
    automated validation passes, there is no unresolved operation, current
    branch is main, the pinned upstream SHA is an ancestor, and the working
    tree is clean. Verify the backup still points to its recorded old SHA.
    Delete the exact ref with an expected-value guard:

    ```sh
    git update-ref -d refs/heads/<this-invocation-backup> <recorded-old-main-SHA>
    ```

    Substitute only the recorded ref name and full SHA. If verification or
    deletion fails, stop and report; never force-delete or sweep backup refs.
    If any prerequisite is unmet, keep the backup and explain why. Never
    create a backup when no rebase is needed.
18. Summarize old/new main and upstream SHAs, resolved conflicts, integration
    fixes, commands and validation results, and whether the backup was deleted
    or retained (with its ref and SHA). Human hardware QA is still required
    before publishing and must not be claimed from automated smoke tests.

Do not run `git commit`, push, force-push, open/update PRs or issues, or post
review replies. Do not claim build or runtime success from Git checks alone.
Human review and a real DJ-setup test are required before publishing.
