// Header file:
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;


namespace compresser{

    typedef struct BufferAndType {
        int * buffer;
        PredType type;
    } BufferAndType;

  class Compresser {
    public:
    Compresser();
    void CompressFile(H5File file, int compressionLevel);
    void DeCompressFile(H5File file, int compressionLevel);
  };
}