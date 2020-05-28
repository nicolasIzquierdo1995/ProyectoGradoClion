#include <iostream>
using namespace  std;

template <typename T>
class h5Array{
private:
    T* ptr;
    int size;

public:
    h5Array(T arr[], int s);
    void print();
};

template <typename T>
h5Array<T>::h5Array(T *arr, int s) {
    ptr = new T[s];
    size = s;
    for (int i = 0; i < size; i++) {
        ptr[i] = arr[i];
    }
}

template <typename T>
void h5Array<T>::print() {
    cout<<"[";
    for (int i = 0; i < size; i++) {
        cout<< " " << *(ptr+i);
    }
    cout<<"]"<<endl;
}
