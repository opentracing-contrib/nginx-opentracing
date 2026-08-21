# AGENTS.md

## Scope

These instructions apply to the entire repository.

## Repository overview

- `opentracing/`: NGINX OpenTracing module sources (`src/`), build config (`config`, `config.make`).
- `test/`: Docker-based integration test environment and the Python test runner.
- `example/`: end-to-end examples for the Jaeger, Zipkin, Datadog, OpenTelemetry,
  and LightStep tracers, from Go, PHP, Lua, and Zoo.
- `doc/`: user-facing documentation (Tutorial.md, Reference.md, images in `data/`).
- `build/`: Docker build assets for producing binary artifacts.
- `Dockerfile`: multi-stage build (Debian + Alpine variants via build args).
- `Dockerfile-openresty`: OpenResty Docker image build.

### Key config files

- `.mise.toml`: tool versions and lint/format tasks.
- `hk.pkl`: hk lint runner config (defines all linters, checkers, pre-commit hooks).
- `.clang-format`: C++ and JavaScript formatting (Google-based style, 80-col, 2-space indent).
- `.editorconfig`: editor defaults (UTF-8, LF, trim trailing whitespace).
- `.markdownlint-cli2.yaml`: Markdown lint rules (dash bullets, 120-char lines, ignore `.github/`).
- `.yamllint.yaml`: YAML lint rules (120-char lines, ignore `.github/`).
- `Makefile`: build, test, lint, and clean targets.
- `renovate.json`: automated dependency updates.

## Preferred workflows

- Read `README.md` first for build, runtime, and tracer setup context.
- Keep changes focused and avoid touching example or test assets unless the task requires it.
- Update documentation when behavior, commands, or supported workflows change.
- Prefer existing Make and `mise` tasks over ad hoc commands.
- Run `mise deps` before first use to install tooling.

## Build, test, and lint

- Install repository tooling with `mise deps`.
- Lint the repository with `mise run lint --no-progress` or `make lint`.
- Auto-fix formatting with `mise run format` when needed.
- Run the integration test suite with `make test`.
- Build the main Docker image with `make docker-image`.
- Build the Alpine variant with `make docker-image-alpine`.
- Build binary artifacts with `make docker-build-binaries`.

### Lint pipeline

`mise run lint` runs `hk check --all`, which executes these linters in order:

1. **trailing-whitespace** -- removes trailing whitespace.
2. **end-of-file-fixer** -- ensures final newline.
3. **check-yaml** -- validates YAML via yamllint.
4. **check-ast** -- validates Python AST.
5. **check-executables-have-shebangs** -- checks shebangs on executables.
6. **check-symlinks** -- validates symlinks.
7. **check-json** -- validates JSON via jq.
8. **mixed-line-ending** -- normalizes line endings.
9. **fix-byte-order-marker** -- removes BOMs.
10. **ruff** -- Python linter (batch mode).
11. **ruff_format** -- Python formatter (depends on ruff).
12. **black** -- Python formatter (depends on ruff_format).
13. **isort** -- Python import sorter (depends on black).
14. **actionlint** -- GitHub Actions workflow linter.
15. **yamllint** -- YAML linter.
16. **markdownlint** -- Markdown linter (`**/*.md`, via markdownlint-cli2).
17. **codespell** -- spell checker (ignores "commitish").
18. **clang-format** -- C++/JavaScript formatter (Google-based style).

Additional checkers: `check-case-conflict`, `editorconfig-checker`.

Pre-commit hooks: `detect-private-key`, `check-added-large-files`,
`check-merge-conflict`, `no-commit-to-branch`, `gitleaks`.

After linting, `mise run postlint` runs `git diff --exit-code` to ensure no
uncommitted changes were introduced by auto-fixers.

## Code style

- **C++/JavaScript**: Google-based style via `.clang-format` (80-col,
  2-space indent, attached braces, sorted includes).
  Format with `clang-format -i`.
- **Python**: 4-space indent. Lint and format via the ruff/black/isort
  pipeline (run through `mise run lint` or `mise run format`).
- **Markdown**: dash-style bullets, 120-char line limit (code blocks and tables exempt). Lint via markdownlint-cli2.
- **YAML**: 120-char max line length. Lint via yamllint.
- **All files**: UTF-8, LF line endings, trailing whitespace trimmed (see `.editorconfig`).
- **Makefile**: tab indentation.

## Editing guidance

- Follow existing Markdown conventions: dash-style bullets and lines that stay within the markdownlint configuration.
- Keep documentation and examples consistent with the current NGINX/OpenTracing terminology used in `README.md` and `doc/`.
- Do not introduce new build, lint, or test tooling unless the repository already uses it.
- When changing module behavior, check whether `README.md`,
  `doc/Reference.md`, `doc/Tutorial.md`, and relevant `example/` content also
  need updates.
- Do not commit generated or compiled artifacts (`.o`, `.so`, `.a`, `.dll`, `__pycache__`).
- Do not edit files under `.github/` workflows unless the task specifically requires CI changes.

## Validation expectations

- For documentation-only changes, run Markdown lint against the touched files: `mise run lint` or `markdownlint-cli2 <files>`.
- For code or configuration changes, run the smallest relevant existing lint/build/test commands before finishing.
- For C++ changes, run `clang-format -i` on modified files and verify
  no diff.
- For Python changes, the ruff/black/isort pipeline runs as part of `mise run lint`.
- CI (`.github/workflows/lint.yml`) runs the same `mise run lint`,
  `actionlint`, and `markdownlint-cli2` checks, so lint locally before
  pushing.
- Review diffs for accidental secrets or generated artifacts before committing.
