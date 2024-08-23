cd build
cmake ..
make -j$(( $(nproc) - 1 ))
cd ..
./build/4DCam