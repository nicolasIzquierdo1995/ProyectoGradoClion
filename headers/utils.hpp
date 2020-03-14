// Header file:
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace utils{
    typedef struct eventData {
        int start;
        int length;
        float mean;
        float stdv;
    } eventData;

    typedef struct compressedEventData {
        int skip;
        int length;
    } compressedEventData;

    typedef struct signalData {
        int signal;
    } signalData;

  class Utils {
    public:
        static DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
        static int GetFilesCount(string path);
        static string* GetFileArray(string path, int fileCount);
        static bool IsInt(DataSet ds);
        static const char* getUIntDtype(int num);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType();
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList();
  };
}