
all: build listener

build:
	mkdir -p build


listener: build covertCLListener.cpp 
	g++ -I../../ext/cxxopts/include/ covertCLListener.cpp -lOpenCL -o build/covertCLListener


clean:
	rm -rf build *~ 
