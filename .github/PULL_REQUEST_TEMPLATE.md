## Summary

<!-- What does this change do? Keep it short. -->

## Related issue

<!-- Example: Closes #123. Remove this section if there is no issue. -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Refactor / cleanup
- [ ] Documentation
- [ ] Build / tooling
- [ ] Other:

## How was this tested?

- [ ] `cmake --preset clang-x64-debug`
- [ ] `cmake --build --preset build-debug`
- [ ] `cmake --build --preset build-release` (if build config or conditional logging changed)
- [ ] Ran `modules/aru/aru.exe` and verified the affected behavior
- [ ] Added or updated a temporary runtime probe, then removed it before committing

Describe the verification in a sentence or two. Include command output or screenshots when useful.

## Checklist

- [ ] Code follows [`docs/coding-style.md`](docs/coding-style.md).
- [ ] New module/partition files are listed in `FILE_SET CXX_MODULES` and wired into the facade.
- [ ] No new compiler warnings were introduced.
- [ ] Public API changes are documented in `docs/api/` and reflected in `README.md` when needed.
- [ ] No temporary test targets, probe files, or build artifacts are committed.
- [ ] Commits have short, descriptive titles.

## Screenshots / logs

<!-- Optional: paste build output, runtime output, or a small usage snippet. -->

## Notes for reviewers

<!-- Optional: platform-specific checks, known limitations, or follow-up work. -->
