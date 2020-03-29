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

    typedef struct EventsAndType {
        compressedEventData* eventBuffer;
        PredType skipType;
        PredType lengthType;
    } EventsAndType;

    typedef struct ReadsAndType {
        long * buffer;
        PredType type;
    } ReadsAndType;


    class Utils {
    public:
        static DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
        static int GetFilesCount(string path);
        static string* GetFileArray(string path, int fileCount);
        static bool IsInt(DataSet ds);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType(PredType skipType,PredType legthType);
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList();
        static PredType getIntType(long* buffer, int count);

    };
}