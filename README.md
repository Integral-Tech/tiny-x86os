<!--
SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Tiny x86 OS
A tiny x86 operating system, referring to the source code of Linux 0.11. 

## Features
- 32-bit protected mode
- Multitasking
- Use Alt+Fn to switch among tty0 ~ tty7
- FAT16 file system (**still have some bugs in ```cp``` and ```rm``` command**)

## How to build?
```bash
mkdir -p images/
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build/ -G "Unix Makefiles"
cmake --build build/ --target all
```

- In order to fully utilize your CPU, you can use the following command to build:
```bash
cmake --build build/ --target all -j `nproc`
```

## Note
- The password is **tinyx86os**
- This operating system is very simple and buggy, so it is only for learning!

## License
- Licensed under the [GNU General Public License, version 3 or any later version](LICENSES/GPL-3.0-or-later.txt).
