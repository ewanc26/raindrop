# Building raindrop

## Upstream source in this repository

raindrop tracks signed Firefox release source. The current import is:

| Item | Value |
| --- | --- |
| Upstream version | Firefox 160.0 |
| Tarball | https://archive.mozilla.org/pub/firefox/releases/160.0/source/firefox-160.0.source.tar.xz |
| SHA-256 | `2b23d2bf6716ea16b45c10aa5c275c78a1d7ff50ad53150809ab27424c58fd81` |
| Signature | Good signature from Mozilla Software Releases `<release@mozilla.com>` |
| Signing subkey | `827E 6586 0867 9618 CD34 9F93 678E 455D 7676 7AA3` |
| Git import | `upstream` branch, tag `firefox-160.0` |

`upstream` contains only pristine imports; raindrop changes are the patch queue
on `main`. To verify a tarball yourself:

```sh
curl -fsSL https://packages.mozilla.org/rpm/firefox/signing-key.gpg -o mozilla-signing-key.gpg
gpg --dearmor < mozilla-signing-key.gpg > mozilla-keyring.gpg
curl -fsSLO https://archive.mozilla.org/pub/firefox/releases/160.0/source/firefox-160.0.source.tar.xz{,.asc}
gpgv --keyring mozilla-keyring.gpg firefox-160.0.source.tar.xz.asc firefox-160.0.source.tar.xz
shasum -a 256 firefox-160.0.source.tar.xz
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
config lives under `build/` and is selected with `MOZCONFIG`.

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

## Status

The stock build and CI job are still to be completed under
[issue #10](https://github.com/ewanc26/raindrop/issues/10). Observed build
times and disk usage will be recorded here once that bootstrap has been timed.
