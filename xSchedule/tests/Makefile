# Builds and runs the schedule unit tests on Linux/macOS: make test
CXX ?= g++
WX_CONFIG ?= wx-config
CXXFLAGS += -std=gnu++20 -O2 -I../../xlights/include -I../../xlights/xLights -I../../xlights/xLights/utils \
            -I../../xlights/xLights/ui -I../../xlights/common -I../../dependencies -I../../dependencies/spdlog/include \
            $(shell $(WX_CONFIG) --cxxflags)
SOURCES = ScheduleTests.cpp ../Schedule.cpp ../City.cpp ../Holidays.cpp

build/xScheduleTests: $(SOURCES) ../Schedule.h ../Holidays.h ../City.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@ $(shell $(WX_CONFIG) --libs std,xml)

test: build/xScheduleTests
	./build/xScheduleTests

clean:
	rm -rf build

.PHONY: test clean
