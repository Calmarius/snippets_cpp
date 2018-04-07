#ifdef UNIT_TEST
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dynamic_array.h"

template <class T>
struct Alloc
{
    T *allocate(size_t n) {return (T*)malloc(n * sizeof(T));}
    T *reallocate(T* buf, size_t n) {return (T*)realloc(buf, n * sizeof(T)); }
    void deallocate(T *buf) {free(buf);}
};

template <class T>
using List = DynArray<T, Alloc<T>>;

int main()
{
    List<int> dyn;
    
    assert(dyn.isAlive());
    
    dyn.add(1);
    dyn.add(2);
    dyn.add(3);
    
    assert(dyn.getCount() == 3);
    
    List<int> dyn2 = dyn;
    
    assert(dyn2.isAlive());
    assert(dyn2.getCount() == 3);
    assert(dyn2.getCapacity() >= dyn2.getCount());
    
    int y = 1;
    for (int x : dyn)
    {
        assert(x == y);
        y++;
    }
    
    assert(dyn2.setCapacity(1));
    assert(dyn2.getLastError() == decltype(dyn)::INVALID_CAPACITY);
    assert(!dyn2.setCapacity(10));
    assert(dyn2.getCapacity() == 10);
    
    assert(dyn[0] == 1);
    assert(dyn[1] == 2);
    assert(dyn[2] == 3);
    {
        int &tmp = dyn[3];
        
        (void)tmp;
        assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    }
    assert(dyn.getLastError() == decltype(dyn)::OK);

    assert(dyn.binarySearch(1));
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.binarySearch(2));
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.binarySearch(3));
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(!dyn.binarySearch(4));
    assert(dyn.getLastError() == decltype(dyn)::OK);

    assert(dyn.contains(1));
    assert(dyn.contains(2));
    assert(dyn.contains(3));
    assert(!dyn.contains(4));
    
    dyn.clear();
    assert(dyn.getCount() == 0);
    
    dyn.add(1);
    dyn.add(2);
    dyn.add(3);
    
    int tmpArray[10] = {0};
    
    dyn.copyTo(tmpArray);
    dyn.copyTo(tmpArray, 5);
    assert(tmpArray[0] == 1);
    assert(tmpArray[1] == 2);
    assert(tmpArray[2] == 3);
    assert(tmpArray[3] == 0);
    assert(tmpArray[4] == 0);
    assert(tmpArray[5] == 1);
    assert(tmpArray[6] == 2);
    assert(tmpArray[7] == 3);
    assert(tmpArray[8] == 0);
    assert(tmpArray[9] == 0);
    
    struct Int2Float
    {
        float operator()(int x) {return (float)x;}
    };

    {
        Alloc<float> ator;
        List<float> floatList = dyn.convertAll<float>([](int x){return (float)x;}, ator);

        assert(floatList.isAlive());
        assert(floatList.getLastError() == decltype(floatList)::OK);
        assert(floatList.getCount() == 3);
        assert(floatList[0] == 1.0);
        assert(floatList[1] == 2.0);
        assert(floatList[2] == 3.0);
    }

    assert(dyn.exists([](int x){return x % 2 == 0;})); // There are even numbers in the collection.
    assert(!dyn.exists([](int x){return x == 5;})); // There is no number 5 in the array.
    
    assert(dyn.find([](int x){return x % 2;})[0] == 1); // The first odd number is 1.
    assert(dyn.find([](int x){return x == 5;}) == nullptr); // There is no 5 in the array.
    
    List<int> matches1 = dyn.findAll([](int x){return x % 2;}); // All odd numbers.
    assert(matches1.getCount() == 2);
    assert(matches1[0] == 1);
    assert(matches1[1] == 3);
    
    List<int> matches2 = dyn.findAll([](int x){return !(x % 2);}); // All even numbers.
    assert(matches2.getCount() == 1);
    assert(matches2[0] == 2);
    
    List<int> matches3 = dyn.findAll([](int x){return x == 5;});
    assert(matches3.getCount() == 0);
    
    dyn.add(4);
    dyn.add(5);
    assert(dyn.findIndex(2, 3, [](int x){return x % 2;}) == 2);
    assert(dyn.findIndex(2, 3, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.findIndex(2, 4, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    assert(dyn.findIndex(5, 0, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    
    assert(dyn.findIndex(2, [](int x){return x % 2;}) == 2);
    assert(dyn.findIndex(2, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.findIndex(5, [](int x){return x % 2;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    
    assert(dyn.findIndex([](int x){return x % 2;}) == 0);
    assert(dyn.findIndex([](int x){return !(x % 2);}) == 1);
    assert(dyn.findIndex([](int){return false;}) == -1);

    assert(dyn.findLastIndex(2, 3, [](int x){return x % 2;}) == 4);
    assert(dyn.findLastIndex(2, 3, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.findLastIndex(2, 4, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    assert(dyn.findLastIndex(5, 0, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    
    assert(dyn.findLastIndex(2, [](int x){return x % 2;}) == 4);
    assert(dyn.findLastIndex(2, [](int x){return x == 6;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::OK);
    assert(dyn.findLastIndex(5, [](int x){return x % 2;}) == -1);
    assert(dyn.getLastError() == decltype(dyn)::INDEX_OUT_OF_RANGE);
    
    assert(dyn.findLastIndex([](int x){return x % 2;}) == 4);
    assert(dyn.findLastIndex([](int x){return !(x % 2);}) == 3);
    assert(dyn.findLastIndex([](int){return false;}) == -1);

    printf("Passed: %s %s\n", __DATE__, __TIME__);

    return 0;
}

#endif