#include <iostream>

int main() {
   int i = 0;
   int k = 0;

   std::cout << i << std::endl;

   for (i = 0; i < 100; i++) {
      std::cout << "&&&&" << std::endl;
      k += i;
   }
}

/*
output →
    ( out-of-band-record )* [ result-record ] "(gdb)" nl
*/
