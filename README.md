# unify – Simple C Include Inliner

`unify` reads one or more C source files, scans them for `#include "file.h"` directives, and expands those includes directly into the output. It resolves files from the current directory or additional include paths provided with `-I`. Already-included files are skipped to prevent duplication and recursion.

## Features

* Expands local includes (`"file.h"`) directly into output
* Supports multiple input files
* Supports multiple include directories using `-I path`
* Prevents duplicate inclusion
* Prints unified source to stdout

## Usage

```
unify file1.c file2.c ...
```

## Add include search paths

```
unify -I include_dir file.c
```

## Example

```
unify -I ./include main.c > combined.c
```

## Notes

* System includes (`<stdio.h>`) are ignored.
* If a referenced file cannot be found, a warning is printed and processing continues.
* Output is written to stdout; redirect as needed.
