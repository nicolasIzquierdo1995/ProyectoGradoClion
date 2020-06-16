#include <iostream>
using namespace  std;

template <typename T>
class h5Array{
  
public:
    T* ptr;
    int size;
    h5Array(){};
    h5Array(T arr[], int s);
    void print();
};


template <typename T>
h5Array<T>::h5Array(T *arr, int s) {
    ptr = arr;
    size = s;
}

template <typename T>
void h5Array<T>::print() {
    cout<<"[";
    for (int i = 0; i < size; i++) {
        cout<< " " << *(ptr+i);
    }
    cout<<"]"<<endl;
}
