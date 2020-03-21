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

    typedef struct EventsAndType {
        compressedEventData * buffer;
        PredType skipType;
        PredType lengthType;
    } EventsAndType;

    typedef struct ReadsAndType {
        int * buffer;
        PredType type;
    } ReadsAndType;


    class Utils {
    public:
        static DataSet* GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName);
        static int GetFilesCount(string path);
        static string* GetFileArray(string path, int fileCount);
        static bool IsInt(DataSet ds);
        static const char* getUIntDtype(int num);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType(PredType skipType,PredType legthType);
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList();
        static PredType getIntType(int* buffer, int count);
  };
}