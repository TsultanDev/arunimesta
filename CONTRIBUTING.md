# Contributing to Arunimesta

Thanks for your interest in Arunimesta! The engine is in early development, so contributions are welcome — from bug reports and documentation fixes to new RHI features.

## Ground Rules

- Follow the [Code of Conduct](CODE_OF_CONDUCT.md). Be respectful and constructive.
- By contributing, you agree that your contribution is licensed under the [MIT License](LICENSE).
- The API and module layout are still unstable. Prefer small, focused changes over large rewrites.
- If you plan a large change (for example a new module or a rendering subsystem), open an issue first so the design can be discussed.

## Prerequisites

| Requirement | Notes |
| :--- | :--- |
| Windows x64 | The provided CMake presets are Windows-only |
| Clang | Must support C++23 named modules; `clang++` in `PATH` |
| CMake | ≥ 3.28, required for `FILE_SET CXX_MODULES` |
| Ninja | Generator used by the presets |
| Vulkan SDK | Required by `aru_rhi` for Vulkan-Hpp headers; dispatch is dynamic, so `vulkan-1.lib` is not linked |
| Git | For branching and pull requests |

See [`docs/building.md`](docs/building.md) for toolchain checks and troubleshooting.

## Build and Run

```bash
cmake --preset clang-x64-debug
cmake --build --preset build-debug
./build/clang-x64-debug/modules/aru/aru.exe
```

Release:

```bash
cmake --preset clang-x64-release
cmake --build --preset build-release
```

If module caches cause strange errors, delete `build/clang-x64-debug` and configure again.

## Making a Change

1. Fork the repository and create a branch from `main`:
   ```bash
   git switch -c rhi/add-swapchain
   ```
2. Keep the change focused. One feature or fix per pull request is easiest to review.
3. Follow [`docs/coding-style.md`](docs/coding-style.md):
   - 4-space indentation, Allman braces, `camelCase` functions/members, `PascalCase` types.
   - Put `#include` directives in the global module fragment, before `export module`.
   - Keep facade files (`lib.cppm`) re-exporting partitions with `export import :partition;`.
   - Use `Result`/`std::expected` for recoverable errors; avoid exceptions at public API boundaries.
   - Keep Win32 code on the Unicode (`...W`) APIs and convert UTF-8 text through `String<char16_t>`.
4. Register every new module/partition in the target's `CMakeLists.txt` under `FILE_SET CXX_MODULES`.
5. Update documentation under `docs/` (and `README.md` when the public API or status changes). If you add an image under `assets/`, register it in [`docs/assets.md`](docs/assets.md).
6. Use short, descriptive commit titles, for example:
   ```text
   rhi: add swapchain
   docs: update surface reference
   fix: keep window open after WM_SIZE
   ```

## Testing

There is no automated test suite yet. Before opening a pull request:

- [ ] Debug build succeeds without new warnings: `cmake --build --preset build-debug`
- [ ] Release build succeeds if the change affects build configuration or conditional logging: `cmake --build --preset build-release`
- [ ] `aru.exe` runs, creates the window/RHI objects, and exits cleanly when the window is closed.
- [ ] Any temporary probe/test target has been removed from `CMakeLists.txt` and the working tree.

For API changes, include a small usage snippet in the PR description so reviewers can see the intended call pattern.

## Reporting Issues

Use the issue templates:

- **Bug report** — something is broken, including build/toolchain problems.
- **Feature request** — a new API, module, or engine capability.

Please search existing issues first. Include your commit hash, Clang/CMake/Ninja versions, Vulkan SDK version (for RHI issues), and the full build or runtime output.

**Do not report security vulnerabilities in a public issue.** See [`SECURITY.md`](SECURITY.md) for private reporting.

## Pull Requests

- Fill out the [pull request template](.github/PULL_REQUEST_TEMPLATE.md).
- Link the related issue with `Closes #123` when applicable.
- Rebase or merge `main` if your branch is behind, and resolve conflicts before requesting review.
- There is no CI workflow yet, so reviewers will build and run the change locally. Clearly note any platform-specific verification you performed.
- Be responsive to review feedback; follow-up commits are fine.

## Project Layout

```text
modules/aru/        executable and public facade
modules/aru_std/    aru.standard (String, Vector, Array, Result, print)
modules/aru_window/ aru.window (Win32 window + event loop)
modules/aru_rhi/    aru.rhi (instance, surface, adapter, device, queue)
docs/               architecture, build guide, coding style, API references
```

See [`docs/README.md`](docs/README.md) for the full documentation index and [`docs/roadmap.md`](docs/roadmap.md) for planned work.
