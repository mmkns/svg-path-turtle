# Releases

.PHONY: release debug all clean

release: build/release/Makefile
	(cd $(dir $<) && make)

debug: build/debug/Makefile
	(cd $(dir $<) && make)

all: release

clean:
	rm -r build

# cmake

build/debug/Makefile: CMakeLists.txt
	mkdir -p $(dir $@)
	(cd $(dir $@) && cmake -DCMAKE_BUILD_TYPE=Debug ../..)

build/release/Makefile: CMakeLists.txt
	mkdir -p $(dir $@)
	(cd $(dir $@) && cmake -DCMAKE_BUILD_TYPE=Release ../..)
