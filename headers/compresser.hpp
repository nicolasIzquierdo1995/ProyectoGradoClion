// Header file:
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace compresser{
  class Compresser {
    public:
    Compresser();
    void CompressFile(H5File file, int compressionLevel);    
  };
}