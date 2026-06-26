---
description: Implement a GitHub issue from the x64dbg Linux-port board (arslan-charyyev/x64dbg) end-to-end — read the issue, branch, find references, implement, build/test, self-review, and commit. Adapted for this repo: GitHub Issues (no Jira, no status transitions), cmkr/CMake/Qt/C++, AStyle formatting, CRLF sources. Stops at commit (never pushes — that's `/create-pr`).
argument-hint: <issue-number-or-URL>
---

Implement the GitHub issue given in `$ARGUMENTS` (an issue number like `42`, or a full GitHub issue URL). The board lives on the fork `arslan-charyyev/x64dbg`. Follow every phase in order. Do not compress, skip, or merge phases. Run `gh` through `direnv exec .` (never interactive login / never ask for a token).

## Phase 1 — Read the issue and gather context

Fetch the issue: `direnv exec . gh issue view <n> --repo arslan-charyyev/x64dbg --json number,title,state,milestone,labels,body,url`. The issue description and acceptance criteria are the source of truth — read them in full. If the issue is closed, stop and confirm with the user before doing anything.

**Blocker gate (replaces Jira refinement).** GitHub issues have no refinement workflow; instead check dependencies and specificity:
- Fetch blockers: `direnv exec . gh api repos/arslan-charyyev/x64dbg/issues/<n>/dependencies/blocked_by`. If any blocker is still **open**, stop and tell the user: "#<n> is blocked by #<b> (`<title>`, still open). Land that first, or reply `continue anyway`." Wait for a separate instruction or explicit `continue anyway`.
- If the issue body has no clear goal or acceptance criteria, say so and ask the user to refine it first rather than guessing scope.

**Build a picture of prior/sibling work so you don't duplicate it:**
- List the milestone's other issues: `direnv exec . gh issue list --repo arslan-charyyev/x64dbg --milestone "<milestone-title>" --state all --json number,title,state`.
- Find what already landed. Sibling work is merged on **upstream** (`x64dbg/x64dbg`), not the fork, and there's no `Closes` link from the fork issue to its PR — so search upstream merged PRs by title/area keywords from the sibling issue: `direnv exec . gh pr list --repo x64dbg/x64dbg --search "<keywords> in:title" --state merged --json number,title,files`. (The most reliable signal is the tree itself: your branch is based on `development`, which already contains every merged sibling — `git log`/`git diff` against it shows their work.) Read the touched files so you build on, not re-add, prior work.

Build a clear picture of: what to build/change, which files/components are involved (engine `ElfBug` / adapter `DbgAdapter` / cross Bridge shim / which `src/gui/Src` widget), expected behavior (inputs→outputs, events, state), explicit constraints/non-goals, and what siblings already added.

## Phase 2 — Sync and checkout the feature branch

The default/integration branch in this repo is **`development`** (not `master`/`main`). Never commit to it directly.

1. **Sync**: `git fetch origin`. Check current branch: `git branch --show-current` — this is the **base ref**.
   - If on `development`: confirm clean tree (`git status --porcelain`); if dirty, stop and tell the user. Then `git merge --ff-only origin/development`. If ff fails (diverged), stop. Base ref is updated `development`.
   - If already on a feature branch: do NOT merge to `development`. Base ref is that feature branch (the new branch stacks on it).
2. **Derive branch name**: `<issue-number>-<title-in-kebab-case>`. Strip any leading scope prefix from the title (e.g. the milestone's `0N)` number, or `Backend:`-style labels), lowercase, replace spaces/punctuation with single hyphens, strip leading/trailing hyphens. Example: issue #12 "Wire DbgMemMap to DbgAdapter" → `12-wire-dbgmemmap-to-dbgadapter`. Check for an existing branch: `git branch -a --list "*<n>-*"`.
3. **Checkout** (these recipes prevent a feature branch from silently tracking `origin/development`, which would make a future push target the integration branch):
   - Exists locally: `git switch <branch>`.
   - Remote only: `git switch <branch>` (auto-tracks `origin/<branch>`).
   - Doesn't exist: `git switch -c <branch>` (when already on the base ref) or `git switch -c <branch> <local-base-ref>`. **Never** pass `origin/<anything>` as start-point without `--no-track`.
   - Verify upstream: `git rev-parse --abbrev-ref --symbolic-full-name @{u} 2>/dev/null`. A fresh feature branch should print nothing (no upstream — correct). An existing remote branch should print `origin/<branch>`. Anything else, especially `origin/development`, is a fail — `git branch --unset-upstream` and stop to tell the user.

## Phase 3 — Find reference implementations

Before writing code, identify the patterns this issue needs and find the nearest existing example. Use `grep` via Bash (the Grep tool is unavailable in this harness) and the Read/Glob tools.

- **Porting a widget?** The pattern is already established: `src/cross/widgets/CMakeLists.txt` (hand-written) lists already-ported `src/gui/Src/*` sources; the Bridge shim (`src/cross/widgets/Bridge.{h,cpp}`, `Configuration.cpp`, `Types.h`) is where missing `Dbg*`/`Bridge*`/`Gui*` API gets implemented or stubbed. Find a comparable already-ported widget to mirror.
- **Engine work?** Mirror the nearest `src/cross/ElfBug/core|process|thread/*` code and its public API in `ElfBug/ElfBug/api/elfbug_api.h`.
- **Adapter/wiring?** Mirror `src/cross/debugger/core/DbgAdapter.{h,cpp}` (implements the widgets' `MemoryProvider`, drives ElfBug via its callback struct, translates events → Qt signals).

List every reference file you found and what it does. **If you can't find a reference for a key pattern, stop and ask the user** to point you at one — don't invent patterns.

**Test-infra check (per `CLAUDE.md`).** ElfBug has a Catch2 suite (`src/cross/ElfBug/tests/`, Linux-only, **disabled by default** behind `-DELFBUG_BUILD_TESTS=ON`) driving the engine against ELF fixtures (`targets/*.cpp`). The widgets/GUI/adapter side has **no test infrastructure**. So: if this issue touches the **ElfBug engine**, you will add/extend Catch2 tests (Phase 4) and run them (Phase 5); find the nearest `TEST_CASE` in `tests.cpp` to mirror. If it's widget/Bridge/adapter/GUI work, there are no tests to write — the verification gate is a clean cross build (Qt5+Qt6) plus AStyle, and keeping the Windows build green by inspection.

Once every pattern has a reference and the plan is unambiguous, write out the reference files + a concrete file-by-file plan, then proceed to Phase 4. Scope/size alone is not a reason to pause. Pause only for a genuine open question (ambiguity in the issue, an unresolved design decision, a reference you're unsure applies).

## Phase 4 — Implement

Follow the Phase 3 references exactly — same structure, conventions, error-handling style. Implement only what the issue requires.

Hard rules:
- Read every file before editing (Read tool, not `cat`).
- Verify signatures exist (`grep` via Bash) before calling them. No guessing APIs/types/params — search first.
- **Edit `cmake.toml`, never the generated `CMakeLists.txt`** (they carry a DO-NOT-EDIT banner) — **except** `src/cross/widgets/CMakeLists.txt`, which is hand-written and IS where you add ported widget sources.
- **Preserve CRLF** in `.cpp`/`.h`. CMake/`.cmake`/`.toml` are LF.
- Touching `src/gui/Src/*` affects the Windows build too — guard platform-specific code (`#ifdef`), prefer making widgets Bridge-API-driven over sprinkling ifdefs, and don't break Windows.
- Prefer replacing Bridge shim stubs with real implementations (often backed by ElfBug) over adding new ifdefs. Grep `src/cross/widgets/Bridge.cpp` for `TODO`.
- No scope creep: no extra features, speculative abstractions, or cleanup beyond what the issue touches.
- **Minimal prose in code** (see the `coding-conventions` skill): comment only the counter-intuitive (a non-obvious *why*, a subtle invariant, a gotcha). Match the surrounding file's comment density; don't narrate.
- Sub-agents spawned this phase use `model: sonnet`.

**Tests.** Only if the issue touches the ElfBug engine: add/extend Catch2 `TEST_CASE`s in `src/cross/ElfBug/tests/tests.cpp` (and a fixture under `targets/` + its `cmake.toml` entry if needed), mirroring the nearest existing case. Cover the acceptance criteria (each behavior + key edge/error cases), not framework internals. For widget/adapter/GUI work, do not write or scaffold tests.

## Phase 5 — Build & run tests

The universal gate is a clean cross build:
```
cd src/cross
cmake -B build -G Ninja -DCMAKE_UNITY_BUILD=ON
cmake --build build
```
If a build fails on a **missing system dependency** (a Qt component, `linuxdeploy`, etc.), stop and ask — do NOT auto-install toolchains.

- **Engine work:** also build with tests and run them — `cmake -B build -G Ninja -DELFBUG_BUILD_TESTS=ON && cmake --build build && (cd build && ctest --output-on-failure)`. All green before Phase 6. Fix the cause (impl or a test you wrote) and re-run; if a failure is a clearly-unrelated pre-existing breakage, say so and proceed.
- **Widget/adapter/GUI work:** the build itself is the gate (it auto-detects Qt5/Qt6). Build the affected target(s) (`--target debugger`, `x64dbg_widgets`, etc.).
- **Windows build** is MSVC-only and can't be built locally — it's covered by CI (`cross.yml` builds the Ubuntu+Windows matrix). Verify your platform guards by inspection.

**Formatting (CI-enforced):** run `uv run --script .github/format/AStyleHelper.py Check`; apply with `... Silent`. Must pass before committing.

## Phase 6 — Critical review

One review owning **correctness against the issue**. Spawn a fresh `general-purpose` sub-agent (`model: sonnet`). Hand it the *map, not your reasoning* (your rationale would anchor it and defeat the fresh-eyes pass):

- **Changed files** — absolute paths; confirm with `git status --short` and `git diff --name-only` against the base ref.
- **Reference pattern files** — the Phase 3 paths (no rationale).
- **Key call sites** — `grep` once for callers/callees of changed symbols; list paths so it can read beyond the diff.
- **Guideline files** — every `CLAUDE.md` between repo root and the changed files (root `CLAUDE.md` covers this repo); the reviewer reads and applies them itself.
- **Full issue title + body verbatim** — the yardstick.

Reviewer contract: read beyond the diff (open each changed file fully, follow callers/callees — correctness is contextual); apply the guideline files (they outrank general best practices); ground every finding in `file:line` + quoted code/observed behavior (no citation, no finding); self-adjudicate (first argue why each candidate is *fine*; keep only survivors); attach confidence (high/medium; drop low unless security/data-loss/crash); no formatting/import/naming-preference nits (AStyle owns those), no "looks good" filler. Look for: correctness against each acceptance criterion (map each to code; a missing/wrong branch is the headline), missing edge/error cases within intended behavior, security, obvious data-consistency hazards in the touched code, scope creep, and prose that restates code. Report findings with severity (P1/P2/P3) + confidence. A clean review is a valid result — don't invent issues.

Fix every P1 and P2 before proceeding. If a fix touches engine tests, re-run Phase 5 and confirm green.

## Phase 7 — Commit (do NOT push)

1. **Sanity-check the tree**: `git status` — no stray files (build dirs, scratch, generated artifacts). `src/cross/build` is gitignored and won't appear. If anything looks unexpected, stop and ask. If engine tests exist and a review fix touched them, confirm green one last time.
2. **Commit**: invoke the `commit` skill. It formats with AStyle before staging, groups by concern (≤~4 files, source separate from tests), and writes a **single imperative header line** matching this repo's style (e.g. "Port MemoryMapView into x64dbg::widgets") — no Conventional-Commits prefix, no body, no footer, no `Co-Authored-By:` / AI-attribution trailer, no issue key in the message. The planning issue stays on the fork and is not linked from the commit (nor from the upstream PR — `/create-pr` omits issue-closing keywords).
3. **STOP — do not `git push`.** Pushing is a separate, explicit step in this project; it happens in `/create-pr` (whose defined plan is push + open PR), not here. Do not open a PR.
4. **Report**: the branch name and the commits created (one SHA + header each), and note that nothing was pushed.
5. **Suggest next commands**: invoke the `suggest-next-commands` skill with `(\`/create-pr\`, \`Push the branch to your fork and open a cross-fork PR against upstream x64dbg/x64dbg:development\`)`.

## Troubleshooting

- If any API call fails with `Extra usage is required for 1M context · run /extra-usage to enable, or /model to switch to standard context`, invoke the `disable-1m-context` skill.
