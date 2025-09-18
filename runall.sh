make clean
./configure --enable-debug --prefix=/opt/bibon
CFLAGS="-g -O0 -fno-omit-frame-pointer -ggdb3" make -j"$(nproc)"
sudo make install
./obj/musl-gcc ~/LeetCode/main.c -g -O0 -fno-omit-frame-pointer -ggdb3 -o ~/LeetCode/main
