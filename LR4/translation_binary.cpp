#include <cstdlib>
#include <string>
#include <algorithm>

extern "C" {

// перевод в двоичную систему
char* translation(long x) {
    if (x == 0) {
        char* result = new char[2];
        result[0] = '0';
        result[1] = '\0';
        return result;
    }

    std::string binary;
    long num = x;
    while (num > 0) {
        binary += (num % 2) ? '1' : '0';
        num /= 2;
    }
    std::reverse(binary.begin(), binary.end());

    char* result = new char[binary.size() + 1];
    std::copy(binary.begin(), binary.end(), result);
    result[binary.size()] = '\0';
    return result;
}

// пузырек
void sort(int* array, int size) {
    bool swapped;
    for(int i = 0; i < size-1; ++i) {
        swapped = false;
        for(int j = 0; j < size-i-1; ++j) {
            if(array[j] > array[j+1]) {
                std::swap(array[j], array[j+1]);
                swapped = true;
            }
        }
        if(!swapped) break;
    }
}

} // extern "C"
