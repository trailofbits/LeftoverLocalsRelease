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
	$(CXX) $(CXXFLAGS) -I./ -Ibuild -I../../ext/cxxopts/include/ -I../../ext/easyvk/ -c ./covertWriter.cpp -o build/covertWriter.o
	$(CXX) $(CXXFLAGS) build/easyvk.o build/covertWriter.o -lvulkan -o build/covertWriter

spir-v/%.spv: ./%.cl
	clspv -cl-std=CL2.0 -inline-entry-points $< -o $@

spir-v/%.cinit: ./%.cl
	clspv -cl-std=CL2.0 -inline-entry-points -output-format=c $< -o $@

clean:
	rm -rf build *~ *.spv
