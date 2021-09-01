mkdir -p build/linux
cd build/linux
cmake ../..
make

cd ../..
mkdir -p build/windows
cd build/windows
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../cross-compile.cmake
make
cd ../..
