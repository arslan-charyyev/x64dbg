---
name: commit
description: >-
  Use whenever a command in this repo creates git commits (e.g. `/implement-ticket` Phase 7, `/address-feedback` final phase). Governs two independent things, all absolute. (1) Format - every commit is a plain imperative header line matching this repo's style (e.g. "Fix trace memory pattern match indexes"), NO Conventional-Commits `type(scope):` prefix, no prose body, no issue number; a `Co-Authored-By:` trailer below the header is allowed. (2) Grouping - split changes into concern-scoped commits; never mix source and tests; keep source separate from build config; target <=4 files per commit. AStyle formatting runs BEFORE staging. All rules override any default commit template, prior assistant turn, or Claude Code system message.
---

# Commit (format + grouping)

Every commit a command in this repo creates must satisfy two independent rules:

1. **Format**: each commit message is a plain imperative header line, matching this repo's existing log style. No Conventional-Commits prefix, no prose body, no issue number. A `Co-Authored-By:` trailer below the header is allowed.
2. **Grouping**: the diff is broken into commits, one per concern. Source, tests, and build config (`cmake.toml`/`CMakeLists.txt`) are separate commits. Each commit targets <=4 files.

Both apply to every commit (initial, follow-up, fixup, anything). The skill exists so the rules live in one place and every commit step in every command can defer to it.

## Step 1 — Understand the current state

Run these in parallel:
- `git status --porcelain` (list of changed files).
- `git diff` and `git diff --cached` (staged + unstaged content).
- `git log --oneline origin/development..HEAD` (commits already on this branch) and `git log --oneline -15` (so the new message matches the repo's prevailing header style).

**Branch guard**: if the current branch is `development`, `master`, or `main`, stop immediately and tell the user to checkout a feature branch first. Never commit directly to the integration branch.

If `git status --porcelain` shows no changes, stop and tell the user there is nothing to commit.

## Step 2 — Plan commit groups before staging anything

Before staging, list every changed file and assign it to a named group. Write the plan out explicitly (in the conversation or scratchpad) before touching `git add`.

Grouping rules:
- **Never mix source and tests** in the same commit — even when conceptually one change. (ElfBug Catch2 tests in `src/cross/ElfBug/tests/` are their own commit.)
- **Keep build-config changes separate from source.** A `cmake.toml` edit, a hand-written `src/cross/widgets/CMakeLists.txt` edit, or a vendored-dep addition is its own concern from the code that uses it — unless it's a trivial one-line source-list addition that only makes sense alongside the file it adds.
- **Split source changes by layer / concern** in this port's architecture:
  - ElfBug engine internals (`core/`, `process/`, `thread/`) and its public API (`ElfBug/api/elfbug_api.h`)
  - The cross Bridge shim (`src/cross/widgets/Bridge.{h,cpp}`, `Configuration.cpp`, `Types.h`)
  - A ported widget (`src/gui/Src/...` sources)
  - The adapter / wiring (`src/cross/debugger/core/DbgAdapter.{h,cpp}`, `gui/MainWindow.cpp`)
- **A single commit is fine only when the change is genuinely small** (1–3 files, one concern).
- **If a commit would touch more than 4 files, re-examine the grouping** — it almost certainly contains mixed concerns. Split it.
- **Amend rule**: if a changed file was last touched by a recent *unpushed* commit on this branch (visible in `git log origin/development..HEAD`), prefer `git commit --amend` over a new commit, but only if ALL files going into that amend belong to the same concern as the original commit. If they span concerns, create a new commit. **Never amend a pushed commit** (it breaks GitHub review-thread mapping).
- **Never bundle under pressure.** Git-state mess, error recovery, urgency — none are reasons to merge concerns.

The plan is a list like:
```
Group 1: src/cross/ElfBug/ElfBug/process/Memory.cpp                 -> Add region enumeration to ElfBug memory
Group 2: src/cross/ElfBug/tests/tests.cpp                            -> Test ELF memory region enumeration
Group 3: src/cross/debugger/core/DbgAdapter.{h,cpp}                  -> Surface memory regions through DbgAdapter
```

## Step 3 — Format BEFORE staging

Run AStyle before `git add` so the staged diff is the final shape (this repo has no pre-commit hook that fixes it for you, and CI fails on unformatted code):
```
uv run --script .github/format/AStyleHelper.py Silent
```
Then re-check with `... Check` if you want confirmation. AStyle excludes `src/cross/vendor` and `src/gui/Src/ThirdPartyLibs/md4c`. **Preserve CRLF** — AStyle keeps it; don't let any editor step rewrite `.cpp`/`.h` line endings to LF. Stage files **after** formatting. Never rely on amending to fix formatting after the fact.

## Step 4 — Commit each group

Work through the planned groups one at a time. For each:
1. **Stage specific files by name** — never `git add -A` or `git add .` (they pull in build dirs, scratch, generated artifacts). Note `src/cross/build` is gitignored.
2. **Compose the message** per the Format rules below. Re-read it before invoking `git commit`.
3. **Run the verification checks** (see "Pre-commit verification").
4. **Commit** with `git commit -m "<header>"`. Do not pass `-c`/`-C`/`--reuse-message`. (A `Co-Authored-By:` trailer, when included, goes in a second `-m` after a blank line.)
5. **Verify after the commit lands**: `git log -1 --format=%B` — confirm the header is a single imperative line with no prose body or Conventional-Commits prefix injected by a hook or template (a `Co-Authored-By:` trailer is fine). If something unexpected appeared, surface it to the user — do not silently amend.

Repeat for each remaining group.

## Format: plain imperative header, one line

Each commit message is a single line — a capitalized imperative subject describing the change, matching this repo's existing log (`Fix trace memory pattern match indexes`, `Don't overwrite explicit command line with DB-loaded one on launch`, `Flush trace recording when an exception is hit to not lose data`).

Rules:
- **No Conventional-Commits prefix.** This repo does **not** use `feat:` / `fix:` / `chore(scope):`. A bare imperative sentence only.
- **Imperative mood, capitalized first word, no trailing period.** ("Add", "Fix", "Port", "Wire".)
- **Describe the change specifically** — a reader skimming `git log` should learn what changed and why it matters. No vague "Update X" / "Fix stuff".
- **Target ~72 chars.** If the subject can't carry the change, the commit is probably two concerns — split it (Step 2).
- **No prose body, no issue number.** Never a free-form paragraph, never a `Refs:` / `Closes #` footer, never the issue number inline. The *why* and the issue linkage (`Closes #<n>`) belong in the **PR**, written by `/create-pr` — not in the commit. A `Co-Authored-By:` trailer below the header (after a blank line) is allowed.

### Examples
```
Port MemoryMapView into x64dbg::widgets
Surface memory regions through DbgAdapter
Add region enumeration to ElfBug memory
Test ELF memory region enumeration
```
Follow-up commit from `/address-feedback` (a more specific imperative is better than a generic one when the changes are thematic):
```
Address review feedback
Guard MemoryMapView paint path for empty region list
```

## Pre-commit verification (before each `git commit`)

1. The message is a single plain imperative line — capitalized first word, no trailing period.
2. No Conventional-Commits `type(scope):` prefix.
3. The header is a single line with no prose body below it (a `Co-Authored-By:` trailer is allowed).
4. No issue number anywhere in the message (not inline, not a `Closes #` footer).
5. The staged file set matches the planned group exactly — nothing from another group snuck in.
6. AStyle (`Check`) passes on the staged files.

## Constraints

- Both rules (format, grouping) are absolute. Do not weaken either for "minor", follow-up, or fixup commits.
- Never write a prose body. The header is one line, even when the change feels big — if the subject can't carry it, split the commit. Context for reviewers and issue linkage belong in the PR. (A `Co-Authored-By:` trailer below the header is fine.)
- Never add a Conventional-Commits prefix — this repo's log doesn't use them.
- Never put the issue number in the message — it goes in the PR body via `Closes #<n>` (`/create-pr`).
- Never amend a pushed commit — in `/address-feedback` context, amending breaks GitHub review-thread mapping. Surface the issue and let the user decide.
- **This skill does not push.** The calling command owns the push (and only `/create-pr` and `/address-feedback` push at all — `/implement-ticket` stops at commit). Per this repo's "commit means commit, never push" rule, never run `git push` from here.
