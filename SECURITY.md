# Security Policy

Arunimesta is experimental software in early development. It is not intended for production use or for processing untrusted input. Please treat any finding accordingly, but still report it privately so it can be addressed.

## Supported Versions

| Version | Supported |
| :--- | :--- |
| Latest commit on `main` | ✅ |
| Older commits | ❌ |
| Tagged releases | Not published yet |

Security fixes are applied to `main`. There are no maintained release branches at this time.

## Reporting a Vulnerability

**Please do not open a public issue for a security vulnerability.**

Use one of these private channels:

1. GitHub private vulnerability reporting (preferred):
   [Report a vulnerability](https://github.com/TsultanDev/arunimesta/security/advisories/new)
2. Email: **sulthan310705@gmail.com** (also used for Code of Conduct reports)

Include as much of the following as possible:

- A description of the vulnerability and its impact.
- The affected commit hash and file/module (for example `renaru/src/adapter.cppm`).
- Steps to reproduce, including compiler/toolchain versions and build preset.
- A proof of concept or minimal reproducer, if available.
- Any suggested fix or mitigation.

We aim to acknowledge reports within **7 days**. Complex issues may take longer to investigate and fix.

## Scope

In scope:

- Source code under `aru/`, `winaru/`, and `renaru/`.
- Build configuration (`CMakeLists.txt`, `CMakePresets.json`) that could produce unsafe binaries or unsafe defaults.
- Documentation that could lead users into insecure usage.

Out of scope:

- Vulnerabilities in third-party tools and dependencies (Clang, CMake, Ninja, Vulkan SDK, MSVC). Report those to their respective vendors.
- Issues that require a debug build with deliberate unsafe flags.
- Social engineering or attacks against contributors' personal accounts.

## Disclosure Policy

- Keep the report private until a fix is available on `main`.
- We will coordinate a public disclosure time with you once the fix is ready.
- If you would like credit, include your preferred name or handle in the report.
- Please do not test vulnerabilities against systems you do not own or have explicit permission to test.

## Notes for This Codebase

- Win32 paths use Unicode (`...W`) APIs; avoid reintroducing ANSI (`...A`) calls for the same window.
- Vulkan entry points are loaded at runtime with Vulkan-Hpp dynamic dispatch; do not assume a statically linked loader.
- There is no networking, file parsing, or shader loading yet, so the current attack surface is limited to the host application and local toolchain.
