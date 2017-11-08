The Lemon Programming Language
==============================

Overview
--------

* `src` source code of Lemon compiler and virtual machine
* `lib` source code of core Lemon library
* `doc` documentations of source code
* `test` test code

Getting Source
--------------

```
$ git clone https://github.com/lemon-lang/lemon.git
```

Build Instructions
------------------
```
make
```
or

```
make DEBUG=0 STATIC=0 USE_MALLOC=0 MODULE_OS=1 MODULE_SOCKET=1
```

* `DEBUG`, debug compiler flags, 0 is off.
* `STATIC`, 0 build with dynamic-linked library, 1 build with static-linked.
* `USE_MALLOC`, stdlib's `malloc` ensure return aligned pointer
* `MODULE_OS`, POSIX builtin os library
* `MODULE_SOCKET`, BSD Socket builtin library

Windows Platform
----------------

Lemon can build on Windows via [TDM-GCC](http://tdm-gcc.tdragon.net/download),
getting source code and use command

```
mingw32-make
```

Porting
-------

`lib/os.c` and `lib/socket.c` are support POSIX and Windows environment.

Contributing
------------

1. Accept [Developer Certificate of Origin](https://developercertificate.org/) by adding `Signed-off-by: Your Name <email@example.org>` to commit log.
2. Check Code Style.
3. Send patch to commiter.


License
-------
Copyright (c) 2017 Zhicheng Wei

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
