// Header file:
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace utils{
  class Utils {
    public:
        Utils();
        DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
  };
}