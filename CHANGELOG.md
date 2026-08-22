# Changelog

All notable changes in this fork relative to
[HenryHu/bsdsensors](https://github.com/HenryHu/bsdsensors) are documented
here. Chip support is otherwise identical to upstream; this fork tracks
upstream via periodic syncs.

## [Unreleased] - 2026-08-22

Differences vs upstream `master` ([f84c98a](https://github.com/HenryHu/bsdsensors/commit/f84c98a), 2026-04-19).

### Added

- **CI producing a ready-to-run FreeBSD binary**: GitHub Actions workflow
  (`.github/workflows/build.yml`) boots a real FreeBSD VM
  ([vmactions/freebsd-vm](https://github.com/vmactions/freebsd-vm)) on every
  push to `master` and on pull requests, builds the project inside it, and
  publishes the result.
  The binary is attached to a rolling [`ci-latest`
  release](https://github.com/spin24/bsdsensors/releases/tag/ci-latest)
  (plus per-commit artifacts with sha256 verification).
- **Single-file static executable**: new CMake option `BSDSENSORS_STATIC_BIN`
  links everything statically (protobuf, glog, gflags, zlib, libc++), so no
  shared libraries or packages are needed at runtime — copy one file and run.
  CI builds gflags/glog/protobuf from source specifically for this.
- Table formatter for sensor output (`PrintSensorValuesTable`).

### Changed

- **Default output** is now an easy-to-read aligned table:
  temperatures, voltages, and fans (RPM, duty cycle, active control method,
  temperature source) in separate sections. The former line-by-line dump is
  gone; `--debug` still prints full detail.
- **Abseil dependency removed** entirely:
  - `lib/banked_io.cc`: timeout/sleep logic ported from `absl::Time` /
    `absl::SleepFor` to `std::chrono` + `std::this_thread`.
  - `src/main.cc`: `ABSL_QCHECK_OK` macro replaced with an explicit status
    check.
  Building now only requires cmake, protobuf, gflags and glog.
- Static linking keeps chip self-registrars via `--whole-archive` (plain
  archive linking silently drops them, making chips "unknown").
- README rewritten (usage, flags, sample output, build instructions);
  LICENSE credits the fork maintainer alongside the original author;
  `.gitignore` extended (`build*/`, `stage/`, core dumps).

### Fixed

- No functional bug fixes relative to upstream; all changes are build
  system, packaging, dependency and presentation related.

[Unreleased]: https://github.com/HenryHu/bsdsensors/compare/master...spin24:bsdsensors:master
