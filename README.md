# cook

a single file build system.  
build `cook.c` once, run it, and it builds your project via a `Cookfile`.  
requires only a C compiler, no external tools needed.

you can ship it with your project.  
just add `cook.c` to your project, and write a `Cookfile`.  

### usage:
```sh
$ cc -o cook cook.c
$ ./cook
```

<br>

## Cookfile examples:

### minimal:
```py
    build(main)
    # runs:
    # cc -o main main.c
```

<br>

### chaining and nesting:
```py
    build(foo).build(bar)
    # is the same as:
    build(foo) {
        build(bar)
    }
    # runs:
    # cc -c -o bar.o bar.c
    # cc -o foo foo.c bar.o
```

<br>

### context inheritance:
when you write a `build()`, it inherits the parent build command's settings.

example:
```py
    build(foo).cflags(-Wall, -Wextra)
    build(bar).cflags(-Wall, -Wextra)
    # is the same as:
    cflags(-Wall, -Wextra)
    build(foo)
    build(bar)
```

both `foo` and `bar` are built with the same flags.
they inherit flags from the current context at the time they're defined.

<br>

### typical Cookfile:

```lua
compiler(gcc)
cflags(-Wall, -Werror, -Wpedantic, -g3)
source_dir(src)
output_dir(build)

build(tester).build(file)

build(cook) {
    build(file, token, lexer, arena, parser, expression,
          statement, interpreter, symbol, build_command, main)
}
```

#### how it works:

* all builds inherit settings from the surrounding context (compiler, flags, dirs)
* `build(tester).build(file)` compiles `file.c` first, then `tester.c` with `file.o`
* `build(cook) { ... }` declares a nested build:
  * this produces the final executable: `build/cook`
  * inside, object dependencies are listed (compiled before linking)
* all source paths are relative to `source_dir`
* input files can be given without extensions, `.c` or `.cpp` is inferred
* no need to manually track `.o` files

<br>

### complex build:

```lua
compiler(g++)
cflags(-Wall, -Wextra, -Werror, -g)
source_dir(src)
output_dir(build)
include_dir(include, imgui, libs/glfw/include, libs/glew/include, libs/glm)
library_dir(libs/glfw/lib, libs/glew/lib)

build(game) {
    link(glfw, GLEW, GL, dl, pthread)
    build(main, input, window, renderer, imgui)
}
```

#### explanation:

* `build(game)` defines the final output: `build/game`
* nested builds are object files compiled before linking
* inherits all settings above (compiler, flags, dirs)

