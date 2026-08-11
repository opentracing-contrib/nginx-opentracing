# AGENTS.md

## Scope

These instructions apply to the entire repository.

## Repository overview

- `opentracing/`: NGINX OpenTracing module sources, configs, and build metadata.
- `test/`: Docker-based integration test environment and the Python test runner.
- `example/`: end-to-end examples for Jaeger, Zipkin, Datadog, OpenTelemetry, PHP, Lua, and Zoo.
- `doc/`: user-facing documentation such as the tutorial and directive reference.
- `build/`: Docker build assets for producing binary artifacts.
- `scripts/`: small repository maintenance scripts.

## Preferred workflows

- Read `README.md` first for build, runtime, and tracer setup context.
- Keep changes focused and avoid touching example or test assets unless the task requires it.
- Update documentation when behavior, commands, or supported workflows change.
- Prefer existing Make and `mise` tasks over ad hoc commands.

## Build, test, and lint

- Install repository tooling with `mise deps`.
- Lint the repository with `mise run lint --no-progress` or `make lint`.
- Auto-fix formatting with `mise run format` when needed.
- Run the integration test suite with `make test`.
- Build the main Docker image with `make docker-image`.
- Build the Alpine variant with `make docker-image-alpine`.
- Build binary artifacts with `make docker-build-binaries`.

## Editing guidance

- Follow existing Markdown conventions: dash-style bullets and lines that stay within the markdownlint configuration.
- Keep documentation and examples consistent with the current NGINX/OpenTracing terminology used in `README.md` and `doc/`.
- Do not introduce new build, lint, or test tooling unless the repository already uses it.
- When changing module behavior, check whether `README.md`,
  `doc/Reference.md`, `doc/Tutorial.md`, and relevant `example/` content also
  need updates.

## Validation expectations

- For documentation-only changes, run Markdown lint against the touched files.
- For code or configuration changes, run the smallest relevant existing lint/build/test commands before finishing.
- Review diffs for accidental secrets or generated artifacts before committing.
