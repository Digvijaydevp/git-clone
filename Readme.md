# AOS Assignment 4

## Features

### Core Functionality
- **Repository Initialization** (`init`)
  - Creates a new .mygit hidden repository with the necessary directory structure
  - Automatically sets up `.mygit` directory with objects, refs, index and HEAD file

### Object Management
- **Hash Object Generation** (`hash-object`)
  - Computes SHA-1 hashes of file contents
  - Optional `-w` flag to write objects to the objects folder
  - Uses zlib compression for stored objects

- **Object Content Viewing** (`cat-file`)
  - `-p` flag: Displays object content
  - `-s` flag: Shows object size
  - `-t` flag: Display object type (blob/tree)

### Tree Operations
- **Tree Creation** (`write-tree`)
  - Generates tree objects representing directory structure
  - Handles file permissions (executable vs non-executable)
  - Maintains directory hierarchy

- **Tree Listing** (`ls-tree`)
  - Lists contents of tree objects
  - `--name-only` flag for displaying only file names
  - Shows permissions, type, hash, and name of entries

### Staging and Committing
- **Staging Files** (`add`)
  - Adds files to the staging area (index)
  - Supports adding individual files and directories

- **Committing Changes** (`commit`)
  - Creates commit objects with:
    - Tree reference
    - Parent commit reference
    - Commit message
    - Author/committer information
    - Timestamp
  - `-m` flag for specifying commit messages

### History and Navigation
- **Commit History** (`log`)
  - Shows commit history with:
    - Commit SHA
    - Parent commit reference
    - Author information
    - Commit message
    - Timestamp
  - Displays commits in chronological order

- **Checkout** (`checkout`)
  - Restores working directory to specified commit state
  - Updates HEAD reference
  - Handles file permissions during restoration

## Execution

- MakeFile is provided inside the project, we can simply use make command to compile the code ./mygit executable to execute the command.


## Notes

- Uses similar concepts and structures to Git
- Two files strict_fstream.hpp and zstr.hpp are added as they are referenced for compression of data.

