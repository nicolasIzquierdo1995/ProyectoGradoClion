// Header file:
#include <string>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace utils{

    typedef struct eventData {
        long start;
        long length;
        float mean;
        float stdv;
    } eventData;

    typedef struct compressedEventData {
        uint8_t skip;
        uint8_t length;
    } compressedEventData;

    typedef struct StdvAndMean {
        float stdv;
        float mean;
    } StdvAndMean;

    class Utils {
    public:
        static DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
        static int GetFilesCount(string path);
        static string* GetFileArray(string path, int fileCount);
        static bool IsInt(DataSet ds);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType();
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList();
        static PredType getIntType(int* buffer, int count);
        static StdvAndMean getStdvAndMean(int* buffer, int start, int length);
        static PredType getCompressedSignalDataType();
    };
}