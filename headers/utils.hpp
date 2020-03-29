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
        long skip;
        long length;
    } compressedEventData;

    typedef struct EventsAndType {
        long* skipBuffer;
        long* lengthBuffer;
        size_t totalSize;
        size_t offset;
        PredType skipType;
        PredType lengthType;
    } EventsAndType;

    typedef struct ReadsAndType {
        void * buffer;
        PredType type;
    } ReadsAndType;


    class Utils {
    public:
        static DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
        static int GetFilesCount(string path);
        static string* GetFileArray(string path, int fileCount);
        static bool IsInt(DataSet ds);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType(size_t size,size_t skipOffset,PredType skipType,PredType legthType);
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList();
        static PredType getIntType(long* buffer, int count);

        static size_t getSize(PredType type);
    };
}