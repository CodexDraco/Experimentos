CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

PruebaVulkan: main.o
	clang++ -o PruebaVulkan main.o $(LDFLAGS)

main.o: main.cpp
	clang++ $(CFLAGS) -c -o main.o main.cpp

.PHONY: test clean

test: PruebaVulkan
	./PruebaVulkan

clean:
	rm -f PruebaVulkan main.o


