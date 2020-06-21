using namespace  std;

template <typename T>
class h5Array{
  
public:
    T* ptr;
    int size;
    h5Array(){};
    h5Array(T arr[], int s);
};


template <typename T>
h5Array<T>::h5Array(T *arr, int s) {
    ptr = arr;
    size = s;
}
