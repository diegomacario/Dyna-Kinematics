.DEFAULT_GOAL := teapong

FILES=ball.cpp camera.cpp collision.cpp finite_state_machine.cpp game.cpp game_object_2D.cpp game_object_3D.cpp main.cpp menu_state.cpp mesh.cpp model.cpp model_loader.cpp movable_game_object_2D.cpp movable_game_object_3D.cpp paddle.cpp pause_state.cpp play_state.cpp renderer_2D.cpp shader.cpp shader_loader.cpp stb_image.cpp texture.cpp texture_loader.cpp win_state.cpp window.cpp

SRC=src
INC=inc
OUT=out
EXEC_NAME=teapong

# Create a string with all the .o files needed to build the game.
# glad.o appended at the end manually because it's a .c file.
# 'patsubst' is a function for text substition defined by GNU Make.
OBJECTS = $(patsubst %.cpp, $(OUT)/%.o, $(FILES)) $(OUT)/glad.o

CXX=g++
CXXFLAGS=-std=c++14 -I $(INC) -O3
LIBS=-l glfw -l assimp -l irrklang
LIBS_HEADERS=-L /usr/local/lib

# To build the game all the .o files in OBJECTS need to have been built and certain directories need to exist
teapong: directories $(OBJECTS)
	$(CXX) $(CXXFLAGS) -I /usr/local/include $(LIBS_HEADERS) $(LIBS) $(OBJECTS) -o $(EXEC_NAME)

# Rule to match the .o targets.
# $@ is the target name (e.g "out/game.o")
# $< is the dependencies (e.g "src/game.cpp")
$(OUT)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule specific to match the glad.o target.
out/glad.o: src/glad.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm out/*.o
	rm teapong

# Rule to ensure out/ directory exists (where .o files are built) before building the game.
.PHONY: directories
directories:
	$(shell mkdir -p $(OUT))
