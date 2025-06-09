# cook

an imperative build system with clean syntax.

### minimal usage:
```py
build(main)
# runs: cc -o main main.c
```

### complex example:
```py
build(game) {
    compiler(g++)
    cflags(-Wall, -Wextra, -Werror, -g)
    include_dir(include, imgui, libs/glfw/include, libs/glew/include, libs/glm)
    library_dir(libs/glfw/lib, libs/glew/lib)
    link(glfw, GLEW, GL, dl, pthread)

    source_dir(src)
    output_dir(build)

    if $DEBUG {
        define(DEBUG)
        build(test)
    }

    build(main, input, window, renderer, imgui)
}
```
### make equivalent:
```make
CXX      = g++
CFLAGS   = -Wall -Wextra -Werror -g
INCLUDES = -Iinclude -Iimgui -Ilibs/glfw/include -Ilibs/glew/include -Ilibs/glm
LDFLAGS  = -Llibs/glfw/lib -Llibs/glew/lib
LDLIBS   = -lglfw -lGLEW -lGL -ldl -pthread

SRC_DIR  = src
BUILD_DIR = build

SRCS = game.cpp main.cpp input.cpp window.cpp renderer.cpp imgui.cpp
OBJS = $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean

ifeq ($(DEBUG),1)
    CFLAGS += -DDEBUG
    SRCS += test.cpp
    OBJS += $(BUILD_DIR)/test.o
endif

all: $(BUILD_DIR)/game

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
    $(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/game: $(OBJS)
    $(CXX) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR):
    mkdir -p $(BUILD_DIR)

clean:
    rm -rf $(BUILD_DIR)
```
