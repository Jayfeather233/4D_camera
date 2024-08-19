cd build
cmake ..
make -j$(( $(nproc) - 1 ))