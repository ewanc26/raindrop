# Building raindrop

## Upstream source in this repository

raindrop tracks signed Firefox release source. The current import is:

| Item | Value |
| --- | --- |
| Upstream version | Firefox 160.0.1 |
| Tarball | https://archive.mozilla.org/pub/firefox/releases/160.0.1/source/firefox-160.0.1.source.tar.xz |
| SHA-256 | `7b9d95ddf24f7bbf5d0d05698bc02a66dac1eca438f80e093c5c76af7648b06d` |
| Signature | Good signature from Mozilla Software Releases `<release@mozilla.com>` |
| Signing subkey | `827E 6586 0867 9618 CD34 9F93 678E 455D 7676 7AA3` |
| Git import | `upstream` branch, tag `firefox-160.0.1` |

`upstream` contains only pristine imports; raindrop changes are the patch queue
on `main`. To verify a tarball yourself:

```sh
curl -fsSL https://packages.mozilla.org/rpm/firefox/signing-key.gpg -o mozilla-signing-key.gpg
gpg --dearmor < mozilla-signing-key.gpg > mozilla-keyring.gpg
curl -fsSLO https://archive.mozilla.org/pub/firefox/releases/160.0.1/source/firefox-160.0.1.source.tar.xz{,.asc}
gpgv --keyring ./mozilla-keyring.gpg firefox-160.0.1.source.tar.xz.asc firefox-160.0.1.source.tar.xz
shasum -a 256 firefox-160.0.1.source.tar.xz
```

Check the key fingerprints against Mozilla's published values before trusting
the key; signing subkeys rotate.

## Prerequisites (macOS)

- A recent macOS release, 8 GB+ RAM, and 30 GB+ free disk space for a build.
- Xcode or the command-line tools: `xcode-select --install`.
- Homebrew.
- Python 3.10–3.14. Firefox 160's `mach` refuses newer versions; if your
  default `python3` is too new (3.15 was too new at the time of writing), use
  [uv](https://docs.astral.sh/uv/):

  ```sh
  curl -LsSf https://astral.sh/uv/install.sh | sh
  uv python install 3.13 --default
  ```

- `ccache` is recommended: `brew install ccache`.

These prerequisites follow Mozilla's
[Building Firefox on macOS](https://firefox-source-docs.mozilla.org/setup/macos_build.html)
guide; consult it for troubleshooting.

## Bootstrap and build

```sh
# From the repository root (the Firefox source tree lives at the root of main).
export MOZCONFIG="$PWD/build/raindrop-unofficial.mozconfig"
./mach bootstrap          # installs remaining dependencies; interactive
./mach build
./mach run
```

`build/raindrop-unofficial.mozconfig` deliberately selects `unofficial`
branding so no Mozilla trademarked names or artwork appear in local builds.
A root-level `.mozconfig` is gitignored upstream, which is why the tracked
config lives under `build/` and is selected with `MOZCONFIG`. The tracked file
is the source of truth; its three active options are:

```sh
mk_add_options MOZ_OBJDIR=@TOPSRCDIR@/obj-raindrop-unofficial
ac_add_options --enable-bootstrap
ac_add_options --with-branding=browser/branding/unofficial
```

`--enable-bootstrap` is required on a clean machine: the release tarball does
not bundle the pinned clang and WASI toolchains, so the first build fetches
them. `--with-branding=browser/branding/unofficial` is why local builds carry
no Mozilla marks; release builds select `browser/branding/raindrop` instead
(issue #11). The tracked file also documents an optional
`--enable-artifact-builds` line for faster iteration after the first build.

(`browser/config/mozconfig` is sourced by the tree itself and supplies
`--enable-project=browser`; the fork config does not need to repeat it.)

To build with raindrop's own names, icons and preferences instead of the
unbranded local defaults:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
./mach build
./mach run
```

See [BRANDING.md](BRANDING.md) for the asset sources and the remaining
trademark verification work.

To produce a distributable archive rather than the development `.app` bundle:

```sh
./mach package
```

## Observed build cost

Measured on macOS with Apple silicon, 10 cores / 24 GB RAM, `ccache` warm and
the `obj-raindrop` object directory:

| Build | Wall time | Object directory |
| --- | --- | --- |
| First full build | 23 min 41 s | 23 GB |
| Later full rebuild | 16 min 40 s | 23 GB |
| Incremental rebuild (a few C++ files) | 16–29 s | 23 GB |

Budget at least 30 GB of free disk for the object directory alone, and 8 GB+
of RAM. A cold build without `ccache` is substantially slower. These numbers
are observations on one machine, not a guarantee; CI runners with less disk
need a larger volume mounted before building.

The pristine `upstream` source (`0ab0e4d32a`, tag `firefox-160.0.1`) was also
built from scratch with this tracked config: `./mach build` exited 0 and
produced `obj-raindrop-unofficial/dist/bin/firefox`, with a 17 GB object
directory. That run was interrupted by a harness timeout after about 10 minutes
and then resumed incrementally, so its total wall time is not a clean
single-pass figure; the 17 GB disk cost is the reliable observation. This is the
local proof that untouched upstream still builds — CI reproduction is the
remaining gap noted below.

## Upstream upgrades

When Mozilla ships a new Firefox release, follow [docs/UPGRADING.md](UPGRADING.md).
The daily `Firefox upgrade check` workflow detects a due upgrade against the
release channel and opens a triage issue; the signed import and rebase remain
reviewed manual steps. The version table above is the pinned source of truth
the check reads.

## Status

The tracked mozconfig's options were verified with a fresh `mach configure` in
an empty object directory. The `upstream` branch is the pristine signed Firefox
160.0.1 import, and that is the source the CI job compiles. `main` is the raindrop
patch queue on top of it: the `netwerk/protocol/atproto/` module and its browser
integration, plus branding, build tooling, docs and repository metadata. A local
`./mach build` on `main` therefore builds the patched tree, not pristine Firefox.
The remaining gap under
[issue #10](https://github.com/ewanc26/raindrop/issues/10) is CI: the
`Build bootstrap` workflow builds the `upstream` branch on every push and pull
request, but a full Firefox build needs 30 GB+ of disk, so the job is gated on
a self-hosted runner selected by the `RAINDROP_BUILD_RUNNER` repository
variable. Without that variable the job is skipped rather than failing on a
hosted runner that cannot fit the build. The source revision itself comes from
the signed `firefox-160.0.1` tag (overridable with `RAINDROP_UPSTREAM_TAG`), as
required by [ADR 0002](decisions/0002-upstream-tracking.md).
