TARGET = mygit

SOURCE = main.cpp

CXX = g++

LIBS = -lssl -lcrypto -lz

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) -o $(TARGET) $(SOURCE) $(LIBS)