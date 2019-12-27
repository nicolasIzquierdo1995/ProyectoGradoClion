// Header file:
#include <string>
#include "/home/osboxes/Desktop/myhdfstuff/HDF5-1.10.5-Linux/HDF_Group/HDF5/1.10.5/include/H5Cpp.h"

using namespace H5;
using namespace std;
namespace utils{
  class Utils {
    public:
        Utils();
        DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
  };
}