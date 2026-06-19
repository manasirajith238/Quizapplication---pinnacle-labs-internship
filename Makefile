CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall
SRC := src/main.cpp
OUT := quiz_server

ifeq ($(OS),Windows_NT)
	OUT := quiz_server.exe
	LDFLAGS := -lws2_32 -lsqlite3
else
	LDFLAGS := -lpthread -lsqlite3
endif

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)

run: $(OUT)
	./$(OUT)

clean:
	rm -f quiz_server quiz_server.exe
