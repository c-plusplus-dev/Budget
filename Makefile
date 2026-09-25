all: build install

mkicon:
	python3 imgcreate.py

build:
	mkdir -p cmake-build-debug
	cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
	cmake --build cmake-build-debug --config Debug

clean:
	@rm -rf dist/* 2>/dev/null || true

install:
	cp -R dist/Budget.app ~/Applications/Nebula
