# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.28.3/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.28.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/piotrwyrw/Seidl

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/piotrwyrw/Seidl

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --cyan "Running CMake cache editor..."
	/opt/homebrew/Cellar/cmake/3.28.3/bin/ccmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --cyan "Running CMake to regenerate build system..."
	/opt/homebrew/Cellar/cmake/3.28.3/bin/cmake --regenerate-during-build -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/piotrwyrw/Seidl/CMakeFiles /Users/piotrwyrw/Seidl//CMakeFiles/progress.marks
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /Users/piotrwyrw/Seidl/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named interpreter

# Build rule for target.
interpreter: cmake_check_build_system
	$(MAKE) $(MAKESILENT) -f CMakeFiles/Makefile2 interpreter
.PHONY : interpreter

# fast build rule for target.
interpreter/fast:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/build
.PHONY : interpreter/fast

src/ast.o: src/ast.c.o
.PHONY : src/ast.o

# target to build an object file
src/ast.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/ast.c.o
.PHONY : src/ast.c.o

src/ast.i: src/ast.c.i
.PHONY : src/ast.i

# target to preprocess a source file
src/ast.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/ast.c.i
.PHONY : src/ast.c.i

src/ast.s: src/ast.c.s
.PHONY : src/ast.s

# target to generate assembly for a file
src/ast.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/ast.c.s
.PHONY : src/ast.c.s

src/io.o: src/io.c.o
.PHONY : src/io.o

# target to build an object file
src/io.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/io.c.o
.PHONY : src/io.c.o

src/io.i: src/io.c.i
.PHONY : src/io.i

# target to preprocess a source file
src/io.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/io.c.i
.PHONY : src/io.c.i

src/io.s: src/io.c.s
.PHONY : src/io.s

# target to generate assembly for a file
src/io.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/io.c.s
.PHONY : src/io.c.s

src/lexer.o: src/lexer.c.o
.PHONY : src/lexer.o

# target to build an object file
src/lexer.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/lexer.c.o
.PHONY : src/lexer.c.o

src/lexer.i: src/lexer.c.i
.PHONY : src/lexer.i

# target to preprocess a source file
src/lexer.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/lexer.c.i
.PHONY : src/lexer.c.i

src/lexer.s: src/lexer.c.s
.PHONY : src/lexer.s

# target to generate assembly for a file
src/lexer.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/lexer.c.s
.PHONY : src/lexer.c.s

src/main.o: src/main.c.o
.PHONY : src/main.o

# target to build an object file
src/main.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/main.c.o
.PHONY : src/main.c.o

src/main.i: src/main.c.i
.PHONY : src/main.i

# target to preprocess a source file
src/main.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/main.c.i
.PHONY : src/main.c.i

src/main.s: src/main.c.s
.PHONY : src/main.s

# target to generate assembly for a file
src/main.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/main.c.s
.PHONY : src/main.c.s

src/parser.o: src/parser.c.o
.PHONY : src/parser.o

# target to build an object file
src/parser.c.o:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/parser.c.o
.PHONY : src/parser.c.o

src/parser.i: src/parser.c.i
.PHONY : src/parser.i

# target to preprocess a source file
src/parser.c.i:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/parser.c.i
.PHONY : src/parser.c.i

src/parser.s: src/parser.c.s
.PHONY : src/parser.s

# target to generate assembly for a file
src/parser.c.s:
	$(MAKE) $(MAKESILENT) -f CMakeFiles/interpreter.dir/build.make CMakeFiles/interpreter.dir/src/parser.c.s
.PHONY : src/parser.c.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... rebuild_cache"
	@echo "... interpreter"
	@echo "... src/ast.o"
	@echo "... src/ast.i"
	@echo "... src/ast.s"
	@echo "... src/io.o"
	@echo "... src/io.i"
	@echo "... src/io.s"
	@echo "... src/lexer.o"
	@echo "... src/lexer.i"
	@echo "... src/lexer.s"
	@echo "... src/main.o"
	@echo "... src/main.i"
	@echo "... src/main.s"
	@echo "... src/parser.o"
	@echo "... src/parser.i"
	@echo "... src/parser.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

