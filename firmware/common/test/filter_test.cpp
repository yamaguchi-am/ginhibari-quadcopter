#include <assert.h>
#include <iostream>
#include "../filter.h"

using std::cout;
using std::endl;

int main() {
    AverageFilter f(3);
    float x;
    assert(!f.Get(&x));
    f.Put(11);
    assert(!f.Get(&x));
    f.Put(22);
    assert(!f.Get(&x));
    f.Put(33);
    assert(f.Get(&x));
    assert(x == 22);
    f.Put(44);
    assert(f.Get(&x));
    cout << x << endl;
    assert(x == 33);
}