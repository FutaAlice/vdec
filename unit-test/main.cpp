#include <iostream>
#include <vdec/vdec.h>
using namespace std;

int main() {
    VDEC dec;
    cout << dec.getConfiguration();

    getchar();
}