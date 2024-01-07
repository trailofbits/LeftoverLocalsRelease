
all: build writer

build:
	mkdir -p build

writer: build covertCLWriter.cpp
	g++ -I../../ext/cxxopts/include/ covertCLWriter.cpp -lOpenCL -o build/covertCLWriter



clean:
	rm -rf build *~ 
