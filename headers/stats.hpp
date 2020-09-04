// Header file:
#include <string>
#include <vector>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace stats {
    class Stats {
    public:
        static void getStats(H5File* file);
        static void getStats(DataSet* dataSet);
    };
}