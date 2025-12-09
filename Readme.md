# mygit — Minimal Git-like implementation

This repository contains a small, educational implementation of core Git ideas (objects, trees, index, commits). The project provides commands such as `init`, `hash-object`, `cat-file`, `write-tree`, `ls-tree`, `add`, `commit`, `log`, and `checkout`.

This README was updated to make it easier to clone the repository, build the project, and get started quickly.

## Quick clone

Use one of the commands below to clone this repository to your machine.

Using HTTPS (recommended for most users):

```
git clone https://github.com/<owner>/<repo>.git
```

Using SSH (if you have an SSH key set up on GitHub):

```
git clone git@github.com:<owner>/<repo>.git
```

Shallow clone (faster, only latest history):

```
git clone --depth 1 https://github.com/<owner>/<repo>.git
```

Replace `<owner>` and `<repo>` with the repository owner and name (for example `username/mygit`).

## Prerequisites

-   Git (to clone and manage the repo)
-   A C++ toolchain (g++, clang, or MSVC) and make for building — or follow the platform-specific notes below.
-   zlib development headers (the project uses zlib compression for objects)

On Windows you can use MSYS2, MinGW, or WSL to get `make` and a POSIX-like build environment. The code was developed with a Unix-like toolchain in mind.

## Build

1. Clone the repo (see above).
2. Change into the project directory:

```
cd <repo>
```

3. Build using make:

```
make
```

This should produce the `mygit` executable (or `mygit.exe` on Windows). If your environment doesn't provide `make`, build with your compiler directly or use an appropriate build script for your platform.

## Usage (examples)

Basic initialization (creates `.mygit`):

```
./mygit init
```

Hash a file and write the object:

```
./mygit hash-object -w path/to/file
```

View an object:

```
./mygit cat-file -p <object-sha>
```

Create a tree from the current index and commit it:

```
./mygit write-tree
./mygit commit -m "Message"
```

List commit history:

```
./mygit log
```

Checkout a commit:

```
./mygit checkout <commit-sha>
```

Note: On Windows replace `./mygit` with `mygit.exe` if needed.

## Features

This section explains the internal layout, objects, and behavior of the project so contributors can understand and extend it.

-   Repository layout

    -   `.mygit/objects/` — object store (zlib-compressed objects keyed by SHA-1)
    -   `.mygit/refs/heads/` — branch refs (plain files containing commit SHAs)
    -   `.mygit/index` — staging index (simplified, lists staged files and their object SHAs)
    -   `.mygit/HEAD` — pointer to the current branch ref (or a raw commit SHA in detached mode)

-   Object model

    -   Supports three core object types: `blob`, `tree`, and `commit`.
    -   Each object is stored as: `<type> <size>\0<raw-bytes>` then zlib-compressed and saved under `objects/` using the SHA-1 hash of the uncompressed data.
    -   `blob` stores file contents. `tree` stores directory entries (mode, name, SHA). `commit` references a tree, optional parent, author/committer and a message.

-   Index (staging)

    -   The index is a minimal staging area mapping file paths to blob SHAs and file modes.
    -   `add` writes or updates entries in the index. `write-tree` reads the index and produces a tree object that reflects the staged state.

-   Refs and HEAD

    -   Branches are simple files under `refs/heads/` containing the commit SHA for the branch tip.
    -   `HEAD` contains either a ref (for example `ref: refs/heads/master`) or a raw SHA for detached HEAD.

-   Implemented commands (quick summary)

    -   `init` — create `.mygit` layout
    -   `hash-object` — compute SHA-1 for a file, `-w` write to object store
    -   `cat-file` — inspect object contents (`-p`, `-s`, `-t`)
    -   `write-tree` / `ls-tree` — make and inspect tree objects
    -   `add` — stage files to the index
    -   `commit` — create commit objects from the current tree and update branch refs
    -   `log` — traverse commits and display history
    -   `checkout` — restore working directory files from a commit

-   Limitations and important differences from real Git

    -   No network support (no push/fetch/clone-over-network implemented here).
    -   No packfiles; objects are stored loose which is fine for small repos but inefficient at scale.
    -   Index and tree formats are simplified and not byte-for-byte compatible with Git's exact formats — this project focuses on concepts rather than full compatibility.
    -   No reflog, no hooks, no submodules, and limited merge/conflict handling.
    -   SHA-1 is used to remain educational, but modern systems should prefer SHA-256; this implementation mirrors the original Git behavior for learning.

-   Testing / quick verification scenario
    1.  `./mygit init`
    2.  Create a file `echo "hello" > hello.txt`
    3.  `./mygit add hello.txt`
    4.  `./mygit write-tree` (optional; `commit` will call this internally)
    5.  `./mygit commit -m "Add hello.txt"`
    6.  `./mygit log` — you should see the new commit listed
    7.  `./mygit cat-file -p <commit-sha>` — inspect commit content; use the tree SHA to `ls-tree` and `cat-file -p` the blob

If those steps succeed, the basic object/index/commit pipeline is working.

Contributors: when changing internals, update this section to reflect new file formats or behaviors so other contributors can follow the implementation details.

## Troubleshooting

-   Build errors about missing zlib: install the `zlib` development package for your platform.
-   Permission issues when checking out: ensure file permissions are supported by your OS and the build preserves execute bits where expected.
-   If `make` is not available on Windows, use WSL or install a developer environment such as MSYS2 or Visual Studio with appropriate build commands.

## Contributing

Contributions are welcome. Please fork the repo, make your changes on a branch, and open a pull request with a clear description of the change.

## Notes

-   The implementation uses helper files such as `strict_fstream.hpp` and `zstr.hpp` for zlib-based compression and stream utilities.
-   This project is an educational reimplementation and intentionally mirrors some Git internal concepts for learning purposes.
