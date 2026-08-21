# Contributing to Spectral Relief

Thank you for helping improve Spectral Relief. Small, focused changes with a
clear audio or visualization benefit are easiest to review.

## Before opening a pull request

1. Open an issue for substantial behavior or architecture changes.
2. Keep the audio path transparent, allocation-free, and lock-free.
3. Add or update tests for behavior changes.
4. Build the VST3 and run the complete test suite.
5. Keep commits focused and explain non-obvious real-time trade-offs.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target SpectralReliefTests SpectralRelief_VST3 -j4
ctest --test-dir build --output-on-failure
```

Changes to rendering should also be exercised using
[docs/testing/ableton-smoke-test.md](docs/testing/ableton-smoke-test.md).

## Style

- Use C++20 and follow the naming and formatting patterns already in the
  surrounding code.
- Types use `PascalCase`; functions and variables use `camelCase`; constants
  use descriptive `camelCase` names.
- Keep units explicit in names such as `frequencyHz`, `durationSeconds`, and
  `averageMilliseconds`.
- Prefer fixed-capacity storage in real-time and rendering pipelines.
- Do not perform allocations, locks, logging, filesystem access, or UI work on
  the audio thread.

## Developer Certificate of Origin

Contributions use the
[Developer Certificate of Origin 1.1](https://developercertificate.org/).
Sign off each commit to certify that you have the right to submit it:

```bash
git commit --signoff
```

The sign-off adds a `Signed-off-by` line using your Git name and email.

By contributing, you agree that your contribution is licensed under
`AGPL-3.0-only`, the same license as the project.
