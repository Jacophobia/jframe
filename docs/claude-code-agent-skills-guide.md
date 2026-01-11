# Claude Code Agent Skills Guide

## Overview

Agent Skills are modular capabilities that extend Claude Code's functionality. Released October 2025, they're Anthropic's recommended approach for equipping Claude with domain-specific expertise.

**Key distinction**: Skills are **model-invoked** — Claude autonomously decides when to use them based on the task and the skill's description. This differs from:
- **Slash commands**: User-invoked (`/command`)
- **Subagents**: Spawned explicitly for parallel work

Think of Skills as onboarding materials for a new hire. Instead of building custom agents for each use case, you package procedural knowledge into composable, shareable capabilities.

---

## Skill Structure

A skill is a directory containing a `SKILL.md` file plus optional supporting resources:

```
my-skill/
├── SKILL.md           # Required: instructions + YAML frontmatter
├── reference.md       # Optional: additional documentation
├── examples.md        # Optional: usage examples
├── scripts/
│   └── helper.py      # Optional: executable code
└── templates/
    └── template.txt   # Optional: templates
```

### SKILL.md Format

```yaml
---
name: pdf-processing
description: Extract text and tables from PDF files, fill forms, merge documents. Use when working with PDFs or document extraction.
---

# PDF Processing

## Quick Start

Extract text:
```python
import pdfplumber
with pdfplumber.open("doc.pdf") as pdf:
    text = pdf.pages[0].extract_text()
```

For form filling, see [forms.md](forms.md).
For API reference, see [reference.md](reference.md).
```

### Field Requirements

| Field | Rules |
|-------|-------|
| `name` | Lowercase letters, numbers, hyphens only. Max 64 chars. |
| `description` | What the skill does AND when to use it. Max 1024 chars. |

The `description` is critical — Claude uses it to decide when to trigger the skill.

---

## Skill Locations

| Scope | Path | Use Case |
|-------|------|----------|
| Personal | `~/.claude/skills/skill-name/SKILL.md` | Your individual workflows, experiments |
| Project | `.claude/skills/skill-name/SKILL.md` | Team workflows, checked into git |
| Plugin | Installed via `/plugin` | Community/marketplace skills |

Create directories:
```bash
# Personal
mkdir -p ~/.claude/skills/my-skill

# Project
mkdir -p .claude/skills/my-skill
```

---

## Progressive Disclosure

Skills use a layered loading strategy to manage context efficiently:

**Level 1 — Startup**: Only `name` and `description` are loaded into the system prompt. Claude knows skills exist but hasn't read them.

**Level 2 — Triggered**: When Claude determines a skill is relevant, it reads the full `SKILL.md` into context.

**Level 3+ — On-demand**: Additional files (`forms.md`, `reference.md`, scripts) are read only when needed.

This means skills can contain unbounded context without bloating every conversation.

```
┌─────────────────────────────────────────────────────────┐
│ System Prompt                                           │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ pdf-processing: Extract text and tables from PDFs  │ │  ← Level 1
│ │ xlsx-analysis: Analyze Excel spreadsheets          │ │
│ └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼ User asks about PDFs
┌─────────────────────────────────────────────────────────┐
│ Claude reads pdf-processing/SKILL.md                    │  ← Level 2
└─────────────────────────────────────────────────────────┘
                           │
                           ▼ User asks about form filling
┌─────────────────────────────────────────────────────────┐
│ Claude reads pdf-processing/forms.md                    │  ← Level 3
└─────────────────────────────────────────────────────────┘
```

---

## Tool Restrictions

Limit which tools Claude can use when a skill is active:

```yaml
---
name: safe-file-reader
description: Read files without making changes. Use for read-only file access.
allowed-tools: Read, Grep, Glob
---

# Safe File Reader

This skill provides read-only file access.
```

When active, Claude can only use `Read`, `Grep`, and `Glob` without permission prompts. Useful for:
- Read-only analysis skills
- Security-sensitive workflows
- Limited-scope operations

If `allowed-tools` is omitted, normal permission rules apply.

---

## Including Executable Code

Skills can bundle scripts that Claude executes as tools. This is useful when:
- Operations are cheaper/faster as code than token generation
- Deterministic behavior is required
- External libraries are needed

Example structure:
```
pdf-processing/
├── SKILL.md
├── forms.md
└── scripts/
    ├── extract_fields.py
    └── fill_form.py
```

Reference from SKILL.md:
```markdown
## Extracting Form Fields

Run the extraction script:
```bash
python scripts/extract_fields.py input.pdf
```
```

Claude runs the script without loading its contents into context. List dependencies in the description:
```yaml
description: Process PDFs. Requires pypdf and pdfplumber packages.
```

---

## Writing Good Descriptions

The description determines when Claude triggers your skill. Be specific about **what** it does and **when** to use it.

### Bad (Too Vague)
```yaml
description: Helps with documents
```

### Good (Specific Triggers)
```yaml
description: Extract text and tables from PDF files, fill forms, merge documents. Use when working with PDF files or when the user mentions PDFs, forms, or document extraction.
```

### Avoiding Conflicts

When multiple skills might overlap, use distinct trigger terms:

```yaml
# Skill 1
description: Analyze sales data in Excel files and CRM exports. Use for sales reports, pipeline analysis, revenue tracking.

# Skill 2  
description: Analyze log files and system metrics. Use for performance monitoring, debugging, system diagnostics.
```

---

## Testing Skills

After creating a skill, test by asking questions that match your description:

```
# If description mentions "PDF files":
Can you help me extract text from this PDF?

# If description mentions "commit messages":
Can you write a commit message for my staged changes?
```

Claude should automatically invoke the skill without explicit mention.

### Debugging

If Claude doesn't use your skill:

1. **Check description specificity** — Include trigger words users would actually say
2. **Verify file path** — Must be exactly `~/.claude/skills/name/SKILL.md` or `.claude/skills/name/SKILL.md`
3. **Validate YAML** — Opening `---` on line 1, closing `---` before content, no tabs
4. **Run with debug mode**:
   ```bash
   claude --debug
   ```

---

## Sharing Skills

### Via Git (Project Skills)

```bash
# Create project skill
mkdir -p .claude/skills/team-workflow
# ... create SKILL.md ...

# Commit
git add .claude/skills/
git commit -m "Add team workflow skill"
git push
```

Team members get skills automatically on `git pull`.

### Via Plugins (Recommended)

1. Create a plugin with skills in the `skills/` directory
2. Add to a marketplace
3. Team installs via `/plugin`

See Claude Code plugin documentation for details.

---

## Examples

### Simple Skill (Single File)

```
commit-helper/
└── SKILL.md
```

```yaml
---
name: generating-commit-messages
description: Generates clear commit messages from git diffs. Use when writing commit messages or reviewing staged changes.
---

# Generating Commit Messages

## Instructions

1. Run `git diff --staged` to see changes
2. Suggest a commit message with:
   - Summary under 50 characters
   - Detailed description
   - Affected components

## Best Practices

- Use present tense ("Add feature" not "Added feature")
- Explain what and why, not how
```

### Read-Only Skill with Tool Restrictions

```yaml
---
name: code-reviewer
description: Review code for best practices and issues. Use when reviewing code, checking PRs, or analyzing code quality.
allowed-tools: Read, Grep, Glob
---

# Code Reviewer

## Checklist

1. Code organization and structure
2. Error handling
3. Performance considerations
4. Security concerns
5. Test coverage

## Instructions

1. Read target files using Read tool
2. Search for patterns using Grep
3. Find related files using Glob
4. Provide detailed feedback
```

### Multi-File Skill

```
data-analysis/
├── SKILL.md
├── pandas-patterns.md
├── visualization.md
└── scripts/
    ├── load_csv.py
    └── generate_report.py
```

**SKILL.md**:
```yaml
---
name: data-analysis
description: Analyze CSV/Excel data, generate statistics, create visualizations. Use when working with tabular data, dataframes, or data analysis tasks. Requires pandas and matplotlib.
---

# Data Analysis

## Quick Start

Load and summarize data:
```python
import pandas as pd
df = pd.read_csv("data.csv")
print(df.describe())
```

For pandas patterns, see [pandas-patterns.md](pandas-patterns.md).
For visualization, see [visualization.md](visualization.md).

## Generating Reports

```bash
python scripts/generate_report.py input.csv output.html
```
```

---

## Skills vs Other Mechanisms

| Mechanism | Invocation | Use Case |
|-----------|------------|----------|
| **Skills** | Model-invoked (automatic) | Domain expertise, complex workflows, unbounded context |
| **Slash Commands** | User-invoked (`/command`) | Quick prompts, explicit actions |
| **Subagents** | Spawned for parallel work | Concurrent execution, isolated tasks |
| **CLAUDE.md** | Always loaded | Project conventions, "should-do" guidance |
| **Hooks** | Deterministic triggers | "Must-do" enforcement, automation |

**When to use Skills**:
- You have procedural knowledge that applies to certain task types
- Context is too large to always include
- You want Claude to automatically apply expertise when relevant

---

## Best Practices

1. **Keep skills focused** — One skill = one capability. Split "Document processing" into "PDF processing", "Excel analysis", etc.

2. **Structure for scale** — Move rarely-used or mutually-exclusive content into separate files. Let Claude load only what's needed.

3. **Think from Claude's perspective** — Monitor how Claude uses your skill. Watch for unexpected paths or overreliance on certain contexts.

4. **Iterate with Claude** — Ask Claude to capture successful approaches into the skill. If it goes off track, ask it to reflect on what went wrong.

5. **Document versions** — Add a version history section to track changes:
   ```markdown
   ## Version History
   - v2.0.0 (2025-10-01): Breaking changes
   - v1.1.0 (2025-09-15): Added features
   - v1.0.0 (2025-09-01): Initial release
   ```

---

## Security Considerations

Skills provide Claude with new capabilities through instructions and code. Malicious skills could:
- Introduce vulnerabilities
- Direct Claude to exfiltrate data
- Take unintended actions

**Recommendations**:
- Install skills only from trusted sources
- Audit third-party skills before use
- Review bundled scripts and dependencies
- Watch for instructions connecting to untrusted external sources

---

## Pre-built Skills

Anthropic provides skills for common document tasks:
- **PDF**: Extract text, fill forms, merge documents
- **Excel**: Analyze spreadsheets, create pivot tables, generate charts
- **PowerPoint**: Create and edit presentations
- **Word**: Create and edit documents

These are available to Pro, Max, Team, and Enterprise users on claude.ai and via the API.

---

## Resources

- [Agent Skills Documentation](https://code.claude.com/docs/en/skills)
- [Engineering Blog: Equipping agents for the real world](https://www.anthropic.com/engineering/equipping-agents-for-the-real-world-with-agent-skills)
- [Skills Cookbook](https://github.com/anthropics/claude-cookbooks/tree/main/skills)
- [Anthropic Skills Repository](https://github.com/anthropics/skills)
