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

all: $(BUILD_DIR)/game

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/game: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

