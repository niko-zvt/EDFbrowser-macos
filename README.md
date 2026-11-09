# EDFbrowser for macOS

Unofficial macOS build of [EDFbrowser](https://gitlab.com/Teuniz/EDFbrowser).

EDFbrowser is Copyright (C) 2007–2026 Teunis van Beelen and is licensed under the GNU GPL v3 (`LICENSE`).

macOS packaging and `build_edfbrowser.sh` are Copyright (C) 2026 Nikolai Zhivotenko, also under the GNU GPL v3. These files were added in 2026 so the program can be built and signed on current macOS. The original copyright notices are unchanged.

This repository follows upstream `master` on the `main` branch. Application docs stay in `README`.

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
