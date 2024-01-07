CXXFLAGS = -std=c++17

SHADERS = $(wildcard ./*.cl)
SPVS = $(patsubst ./%.cl,spir-v/%.spv,$(SHADERS))
CINITS = $(patsubst ./%.cl,spir-v/%.cinit,$(SHADERS))

.PHONY: all clean easyvk writer

all: build easyvk writer

build:
	mkdir -p build

easyvk: build ../../ext/easyvk/easyvk.cpp ../../ext/easyvk/easyvk.h
	$(CXX) $(CXXFLAGS) -I./ -c ../../ext/easyvk/easyvk.cpp -o build/easyvk.o

writer: build easyvk $(SPVS) $(CINITS)
	$(CXX) $(CXXFLAGS) -I./ -Ibuild -I../../ext/easyvk/ -I../../ext/cxxopts/include/ -c ./covertListener.cpp -o build/covertListener.o
	$(CXX) $(CXXFLAGS) build/easyvk.o build/covertListener.o -lvulkan -o build/covertListener

spir-v/%.spv: ./%.cl
	clspv -cl-std=CL2.0 -inline-entry-points $< -o $@

spir-v/%.cinit: ./%.cl
	clspv -cl-std=CL2.0 -inline-entry-points -output-format=c $< -o $@

clean:
	rm -rf build *~ *.spv 
