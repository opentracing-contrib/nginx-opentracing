# AGENTS.md

## Scope

These instructions apply to the entire repository.

## Repository overview

- `opentracing/`: NGINX OpenTracing module sources (`src/` – 25 C++ files),
  build config (`config`, `config.make`).
- `test/`: Docker-based integration test environment (`Dockerfile-test`,
  `Dockerfile-backend`, `environment/grpc`) and Python runner
  (`nginx_opentracing_test.py`).
- `example/`: end-to-end examples for Jaeger, Zipkin, Datadog, OpenTelemetry,
  and LightStep – Go (`go/`), PHP (`php/`), Lua (`lua/`), Zoo (`zoo/`), trivial
  (`trivial/ubuntu-x86_64`).
- `doc/`: user-facing documentation (Tutorial.md, Reference.md, images in `data/`).
- `build/`: Docker build asset for portable binaries (`build/Dockerfile`,
  `scratch` `export` stage → `ngx_http_opentracing_module.so`).
- `Dockerfile`: multi-stage build (Debian default, Alpine via
  `BUILD_OS=alpine`; args `OPENTRACING_CPP_VERSION`, `JAEGER_CPP_VERSION`,
  `GRPC_VERSION`, `ZIPKIN_CPP_VERSION`, `DATADOG_VERSION`).
- `Dockerfile-openresty`: OpenResty image build (module via
  `--add-dynamic-module=/src/opentracing`).

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
- Run `mise install` (or `mise deps` – CI entrypoint, also installs pip deps via `uv`)
  before first use to install tooling.

## Build, test, and lint

- Install repository tooling with `mise deps` (CI) or `mise install`.
- Lint the repository with `mise run lint --no-progress` (`hk check --all`) or
  `make lint`.
- Auto-fix formatting with `mise run format` (`hk fix --all`) when needed.
- Run the integration test suite with `make test` (builds `test/Dockerfile-test`,
  `test/Dockerfile-backend`, `test/environment/grpc/Dockerfile` and runs
  `nginx_opentracing_test.py` with `LOG_DIR=test/test-log`).
- Build the main Docker image with `make docker-image`.
- Build the Alpine variant with `make docker-image-alpine`.
- Build portable binary artifacts with `make docker-build-binaries`
  (`build/Dockerfile`, `--platform linux/amd64`, output `out/`).
- Clean test logs with `make clean`.

### Lint pipeline

`mise run lint` runs `hk check --all`, which executes the `linters` group from
`hk.pkl:7-46`. `mise run format` runs `hk fix --all` to auto-fix fixable steps.

1. **actionlint** -- validates GitHub Actions workflows.
2. **check-ast** -- validates Python AST.
3. **check-executables-have-shebangs** -- checks shebangs on executables.
4. **check-json** -- validates JSON via jq.
5. **check-shebang-scripts-are-executable** -- ensures shebang scripts are executable.
6. **check-symlinks** -- validates symlinks.
7. **clang-format** -- C++/JavaScript formatter (Google-based style).
8. **destroyed-symlinks** -- detects broken symlinks.
9. **end-of-file-fixer** -- ensures final newline.
10. **fix-byte-order-marker** -- removes BOMs.
11. **fix-smart-quotes** -- fixes smart quotes.
12. **mixed-line-ending** -- normalizes line endings.
13. **pkl-lint** -- formats PKL via `pkl_format`.
14. **trailing-whitespace** -- removes trailing whitespace.
15. **yamllint** -- validates YAML.
16. **check-case-conflict** -- detects filename case conflicts.
17. **editorconfig-checker** -- validates against `.editorconfig`.
18. **ruff** -- Python linter (batch mode).
19. **ruff_format** -- Python formatter (depends on ruff).
20. **black** -- Python formatter (depends on ruff_format).
21. **isort** -- Python import sorter (depends on black).
22. **markdownlint** -- Markdown linter (`**/*.md`, via markdownlint-cli2).
23. **codespell** -- spell checker (ignores "commitish").

Pre-commit (`git commit` after `hk install`) runs the `pre-commit` hook
(`hk.pkl:56-71`): the `linters` group plus:

- **postlint** -- `mise run postlint` (`git diff --exit-code`, `exclusive = true`)
  ensures auto-fixers left no diff.
- **precommit** group (`hk.pkl:48-54`): `detect-private-key`,
  `check-added-large-files`, `check-merge-conflict`, `no-commit-to-branch`,
  `betterleaks`.

`hk check` / `hk fix` without `--all` operate on changed files only.

## Code style

- **C++/JavaScript**: Google-based style via `.clang-format` (80-col,
  2-space indent, attached braces, sorted includes).
  Format with `clang-format -i` or `mise run format`.
- **Python**: 4-space indent. Lint and format via the ruff/black/isort
  pipeline (run through `mise run lint` or `mise run format`).
- **Markdown**: dash-style bullets, 120-char line limit (code blocks and tables exempt).
  Lint via markdownlint-cli2.
- **YAML**: 120-char max line length. Lint via yamllint.
- **PKL**: formatted via `pkl-lint` (`pkl_format`).
- **All files**: UTF-8, LF line endings, trailing whitespace trimmed, final
  newline (see `.editorconfig`).
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

- For documentation-only changes, run Markdown lint against the touched files:
  `mise run lint` or `markdownlint-cli2 <files>`.
- For code or configuration changes, run the smallest relevant existing
  lint/build/test commands before finishing.
- For C++ changes, run `clang-format -i` on modified files (or `mise run format`)
  and verify no diff.
- For Python changes, the ruff/black/isort pipeline runs as part of `mise run lint`.
- CI (`.github/workflows/lint.yml`) runs three jobs – `checks`
  (`mise run lint --no-progress`), `actionlint` (reviewdog), and `markdown-lint`
  (markdownlint-cli2) – so lint locally before pushing.
- Review diffs for accidental secrets or generated artifacts before committing.
