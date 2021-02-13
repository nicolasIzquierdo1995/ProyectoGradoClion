// Header file:DeCompressFile
#include <string>
#include "H5Cpp.h"

using namespace std;
#ifndef H5_NO_NAMESPACE
  using namespace H5;
#endif
namespace inputOutput{
  typedef struct Arguments {
    H5File * file;
    int compressionLevel;
    bool isOk;
    bool compress;
    string fileName;
  } Arguments;

  class InputOutput {
    public:
    static Arguments* ProcessArguments(int argc, char* argv[]);
    static void PrintOutput(string msg);
    static int GetDataSetInput(int count);
  };
}