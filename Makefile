# SystemC installation directory - adjust this path
SYSTEMC = /usr/local/systemc-2.3.3

# Architecture and library settings
SYSTEMC_ARCH = linux64
LIB_DIRS = $(SYSTEMC)/lib-$(SYSTEMC_ARCH)

# Include directories
INCLUDE_DIRS = -I. -I$(SYSTEMC)/include

# Compiler settings
CXX = g++
CXXFLAGS = -g $(INCLUDE_DIRS) -std=c++17 -Wall

# Source files - adjusted to your filename
SOURCES = risc_processor.cpp

# Libraries
LIBS = -lsystemc -lstdc++ -lm

# Target executable
TARGET = risc

# Dependencies
DEPENDENCIES = \
		Makefile \
		$(SOURCES)

# Default target
all: $(TARGET)
		./$(TARGET)

# Linking
$(TARGET): $(DEPENDENCIES)
		$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) -L$(LIB_DIRS) $(LIBS)

# Clean target
clean:
		rm -f $(TARGET) *.vcd *.dat

.PHONY: all clean