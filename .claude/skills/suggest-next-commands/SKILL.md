---
name: suggest-next-commands
description: >-
  Standardize how this repo's commands (`/implement-ticket`, `/create-pr`, `/address-feedback`, `/cleanup-merged`) print "what to run next" at the end of their reports. Takes a list of `(command, description)` pairs and emits a consistent markdown bullet list under a `## Suggested next commands` heading — one bullet per command, command wrapped in backticks, em-dash separating it from a short description. Skips the section entirely when the list is empty — never prints an empty heading.
---

# Suggest next commands

A small formatting / inclusion-rules skill. This standardizes each command's tail "next steps" output to a single bullet-list shape so the user sees the same format every time and can copy any command in one click.

## Inputs (from the caller)

An **ordered list** of suggestions. Each suggestion is a `(command, description)` pair:
- `command` — the exact copy-pasteable command, including arguments. Always pass the resolved form (e.g. `/create-pr` or `/cleanup-merged 7`, not `/cleanup-merged <PR#>`). The user should be able to copy it into the prompt with no edits.
- `description` — one short line explaining what the command does next or the condition under which the user would pick it up. May include inline `code spans` for identifiers (issue numbers, paths, branch names). No bold, no emphasis, no nested lists.

If the list is empty, the skill outputs nothing — **not** an empty heading, **not** a "no suggestions" line. Total silence. The caller's report ends with whatever came before.

## Output format

When at least one suggestion is passed, emit a `## Suggested next commands` heading followed by a markdown bullet list. Each bullet is `- \`<command>\` — <description>` (em-dash, U+2014, single spaces on either side).

Rules:
- **Always wrap the command in backticks** — keeps it monospaced and most renderers treat inline code as a non-breaking token so the command doesn't wrap mid-string.
- **Use an em-dash separator** (`—`), not a hyphen, between command and description.
- **One bullet per command.** Never split a command across bullets; never combine two into one bullet.
- **Preserve the caller's order** — the caller knows the priority.
- **Plain dash bullet** (`- `, not `* ` or `1. `).

### Example — two bullets

Input list:
- `/address-feedback 7`, `Once reviewers leave comments, triage and address them`
- `/cleanup-merged 7`, `Once the PR is merged, delete the local + remote branch and fast-forward development`

Output:
```
## Suggested next commands

- `/address-feedback 7` — Once reviewers leave comments, triage and address them
- `/cleanup-merged 7` — Once the PR is merged, delete the local + remote branch and fast-forward development
```

### Example — single bullet

Input list:
- `/create-pr`, `Push the branch and open a PR (links #12 via "Closes")`

Output (a single bullet still gets the heading — consistency beats minor visual weight):
```
## Suggested next commands

- `/create-pr` — Push the branch and open a PR (links #12 via "Closes")
```

### Spacing around the block

Place exactly one blank line above the `## Suggested next commands` heading and one blank line below the last bullet. Do not add prose between the heading and the bullet list, or below the list — the block is the end of the report.

## What's a good suggestion vs. clutter

The caller decides what to put in the list. These rules guide what belongs:
- **Include**: the immediately next thing the user is likely to run, or a clearly-conditional next step keyed to an event ("once reviewers leave comments…", "once the PR is merged…"). The `description` carries the precondition.
- **Include** every entry of a true fan-out (e.g. N independent next steps that all need running).
- **Exclude** the entire downstream chain: after `/implement-ticket` finishes, suggest `/create-pr` (the immediate next), not `/create-pr` + `/address-feedback` + `/cleanup-merged`. The downstream commands surface their own next-steps in turn.
- **Exclude** anything the user already knows is coming and doesn't need re-stated.
- **Exclude** suggestions with no clear copy-pasteable command. "Go look at this in the GitHub UI" belongs in the prose above the list, not in the list.

When in doubt, prefer fewer bullets. A short list the user actually reads beats a long one they skim past.

## Edge cases

- **Conditional bullets** included only when some condition holds are fine — the caller decides. The skill does no filtering; it formats whatever list it's given.
- **Long descriptions** — keep each to one short sentence. If it would wrap past two terminal lines, shorten it.
- **Single-bullet lists** are fine — don't drop the heading or the bullet.
- **Commands with no argument context** — when the command takes an optional argument the caller doesn't know, prefer the bare form (e.g. `/create-pr`) over inventing one.

## Constraints

- Never invent a `Suggested next commands` section the caller didn't ask for. Empty list → print nothing.
- Never reorder, deduplicate, or filter the caller's list.
- Never wrap the bullet list in another block (callout, code fence, etc.).
- Never add prose between the heading and the bullets or below the bullets — the list is the end of the report.
