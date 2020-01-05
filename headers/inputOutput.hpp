// Header file:
#include <string>
#include "H5Cpp.h"

using namespace std;
#ifndef H5_NO_NAMESPACE
  using namespace H5;
#endif
namespace inputOutput{
  typedef struct Arguments {
    H5File file;
    bool multiThreading;
    int compressionLevel;
    bool isOk;
    string errorMessage;
  } Arguments;

  class InputOutput {
    public:
    static Arguments* ProcessArguments(int argc, char* argv[]);
  };
}