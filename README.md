# EDFbrowser for macOS

Unofficial macOS build of [EDFbrowser](https://gitlab.com/Teuniz/EDFbrowser) by Teunis van Beelen.

This repository follows upstream `master` on the `main` branch. The added file is `build_edfbrowser.sh`. Application docs stay in `README`. License: GNU GPL v3 (`LICENSE`).

Upstream does not ship a maintained macOS release. The script fills the gaps on current macOS:

- Qt 5 from Homebrew (`qt@5`)
- removal of the obsolete AGL framework, which current macOS SDKs no longer provide
- a local comment-out of `#error "configuration error"` in `global.h` when `_FORTIFY_SOURCE` is set (the source file in git is unchanged)
- ad-hoc signing with 16 KB pages, so the app is not killed on launch by macOS 27

## Build

Install Xcode Command Line Tools and Qt 5, then build from this directory:

```bash
xcode-select --install
brew install qt@5
./build_edfbrowser.sh
```

| Command | Result |
| --- | --- |
| `./build_edfbrowser.sh` | Build and open the app |
| `./build_edfbrowser.sh --no-open` | Build only |
| `./build_edfbrowser.sh --install` | Build and copy to `/Applications` |
| `./build_edfbrowser.sh --dmg` | Build a DMG in this directory |

An ad-hoc signature is not notarized. On first launch, right-click `EDFbrowser.app` and choose Open.

## Update from upstream

`upstream` is `https://gitlab.com/Teuniz/EDFbrowser.git`. `origin` is this GitHub repository.

```bash
git fetch upstream --prune --tags
git checkout main
git merge upstream/master
git push origin main
git push origin --tags
```
