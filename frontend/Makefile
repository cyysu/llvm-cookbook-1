all:
	clang++ -g -O3 toy.cpp \
	`llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` \
	 -o toy -rdynamic
.PHONY: clean
clean:
	rm toy
