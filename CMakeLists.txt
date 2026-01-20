cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=readability-* -header-filter=.*)

project(svg_path_turtle VERSION 0.7)
set(PROJECT_VERSION_STRING "0.7-alpha")

add_executable( svg_path_turtle
		    src/svg_path_turtle/main.cpp
		    src/svg_path_turtle/Options.cpp
		    src/svg_path_turtle/Parser.cpp
		    src/svg_path_turtle/ASTNode.cpp
		    src/svg_path_turtle/Tokenizer.cpp
		    src/svg_path_turtle/Engine.cpp
		    src/svg_path_turtle/Debug.cpp
		    src/svg_path_turtle/Messages.cpp
		    src/svg_path_turtle/BasicSVG.cpp
		    src/svg_path_turtle/FileUtil.cpp
		    src/svg_path_turtle/Turtle.cpp
		    src/svg_path_turtle/OstreamTurtle.cpp
		    src/svg_path_turtle/Matrix.cpp
		    src/svg_path_turtle/DoubleToString.cpp )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/svg_path_turtle/version.h.in
	       ${CMAKE_CURRENT_BINARY_DIR}/version.h
	       @ONLY)

target_include_directories(svg_path_turtle
			    PRIVATE "src/parser_starter_kit"
			    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(svg_path_turtle PRIVATE -Wall)

# This copy makes it easier to run the tests and such.  It could be a symlink
# on linux, but has to be a copy on windows.
# TODO: change to copy_if_newer when switching to cmake 4.2
add_custom_target(CopyToBaseDir ALL
		  COMMAND ${CMAKE_COMMAND} -E copy_if_different
						# FROM
						${CMAKE_CURRENT_SOURCE_DIR}/lib/library.svgt 
						$<TARGET_FILE:svg_path_turtle>
						# TO
						${CMAKE_CURRENT_SOURCE_DIR})
