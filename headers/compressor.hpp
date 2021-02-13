// Header file:NATIVE_INT
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;


namespace compressor{

  class Compressor {
    public:
    Compressor();
    void CompressFile(H5File* file, int compressionLevel);
    void DeCompressFile(H5File* file, int compressionLevel);
  };
}