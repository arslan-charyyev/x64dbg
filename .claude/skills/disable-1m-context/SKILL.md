---
name: disable-1m-context
description: Use when an API call in any command fails with `Extra usage is required for 1M context · run /extra-usage to enable, or /model to switch to standard context`. The user's account does not have 1M-context extra usage enabled, but Claude Code is auto-promoting the bare model ID to its `[1m]` variant. Tells the user how to disable that auto-promotion via the `CLAUDE_CODE_DISABLE_1M_CONTEXT` environment variable, persistently, on their shell of choice.
---

# Disable 1M-context auto-promotion

When an API call inside a command fails with:

> Extra usage is required for 1M context · run /extra-usage to enable, or /model to switch to standard context

the cause is that Claude Code is auto-promoting the bare model ID to its `[1m]` variant (e.g. `claude-opus-4-8` -> `claude-opus-4-8[1m]`), and the user's account does not have 1M-context extra usage enabled.

Pinning the model in `settings.json` (`"model"`) or in command frontmatter (`model:`) does not help on its own — the auto-promotion happens *after* the model ID is read. The `CLAUDE_CODE_DISABLE_1M_CONTEXT=1` environment variable is the only reliable kill switch.

## What to tell the user

Set `CLAUDE_CODE_DISABLE_1M_CONTEXT=1` persistently, then fully restart Claude Code. Give them the snippet that matches their shell:

- **Linux / macOS** (bash, zsh, etc.): add `export CLAUDE_CODE_DISABLE_1M_CONTEXT=1` to the appropriate rc file — `~/.bashrc`, `~/.zshrc`, `~/.config/fish/config.fish`, `~/.bash_profile` on macOS bash, etc.
- **Windows PowerShell**: add `$env:CLAUDE_CODE_DISABLE_1M_CONTEXT = '1'` to `$PROFILE`.
- **Windows cmd**: run `setx CLAUDE_CODE_DISABLE_1M_CONTEXT 1` once.

The env var only takes effect in newly-launched Claude Code sessions, so the restart is required.

## Constraints

- Never edit the user's rc file or `$PROFILE` without explicit confirmation first. Show the snippet, name the exact file you would append to, and ask before writing — these files may be under version control or follow conventions you cannot see. Once the user confirms, editing is fine.
- Never suggest `/extra-usage` as the fix unless the user has explicitly said they want to enable 1M context on their account. The default assumption is that they hit the error *because* they do not want it enabled.
