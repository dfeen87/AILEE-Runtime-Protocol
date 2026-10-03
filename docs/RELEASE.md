# Release Process (Draft)

## Current release: 38.0.0

Version 38.0.0 is the BEDROCK engineering baseline. Several behavioral contracts
are intentionally stricter: malformed trust evidence, non-finite telemetry, invalid
governance configuration, and mutated ledger content are rejected. The `/v37` API
route remains available. See `V38_BEDROCK_RELEASE.md` for the release contract and
remaining external-validation requirements.

This project is currently in a research and prototype stage. The steps below define a future-facing, lightweight release process to improve consistency and
traceability as the codebase matures.

## Versioning

We follow **Semantic Versioning** (`MAJOR.MINOR.PATCH`):
- **MAJOR**: incompatible API or behavior changes
- **MINOR**: backward-compatible functionality
- **PATCH**: backward-compatible bug fixes

The repository is now on a stable major release line. Version 38.0.0 contains
incompatible malformed-input and trust-decision behavior changes, while preserving
valid public interfaces and the existing architecture.

## Release Checklist

1. Update `CHANGELOG.md` with a summary of changes.
2. Ensure `docs/BUILD.md` steps pass on supported platforms.
3. Run tests (`ctest`) and record results.
4. Tag the release:
   ```bash
   git tag -a vX.Y.Z -m "Release vX.Y.Z"
   git push origin vX.Y.Z
   ```
5. Create a GitHub release with notes and any artifacts.

## Supported Platforms

Document the currently supported compilers and operating systems in `docs/BUILD.md`.
