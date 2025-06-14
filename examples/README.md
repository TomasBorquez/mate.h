# Examples
To run all the examples all you need is a copy of mate.h on your project, and a C compiler, we will mostly use [gcc](https://gcc.gnu.org/) in these examples
available for both linux and windows (MinGW). And only if you are on **windows** [ninja](https://github.com/ninja-build/ninja/releases) (you can download the latest
win release by clicking on `ninja-win.zip`), for **linux** we use [samurai](https://github.com/michaelforney/samurai) a ninja rewrite in C, this is bootstrapped and compiled
on the first run of your mate build after that it's cached.

You can run all examples it with:
```bash
# Linux / FreeBSD
"gcc mate.c -o mate && ./mate"

# Windows
"gcc mate.c -o mate.exe && ./mate.exe"
```

Mate rebuilds itself if there were any changes on `mate.c`, so after compiling once you can just run it with `./mate`.

### Advice
A build system for very simple projects is overkill, I recommended only moving to a build system if you have many vendor packages or
many C files, this will make compile times usually faster and make your project more organized. In the mean time you can look into
[unity builds](https://en.wikipedia.org/wiki/Unity_build) which are way simpler, require no build system and will make you happier :)
since you'll have to use no build system
