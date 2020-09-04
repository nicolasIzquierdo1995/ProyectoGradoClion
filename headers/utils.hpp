// Header file:
#include <string>
#include <vector>
#include "H5Cpp.h"

#ifndef H5_NO_NAMESPACE
using namespace H5;
#endif

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
        float mean;
        float stdv;
    } compressedEventData;

    class Utils {
    public:
        static CompType getEventDataType();
        static CompType getCompressedEventDataType();
        static PredType getSignalDataType();

        static vector<DataSet>* listDatasets(string name,H5File* file,string path);

        static DSetCreatPropList* createCompressedSetCreatPropList(int size);
        static DSetCreatPropList* createDecompressedSetCreatPropList(int size);
        static DSetCreatPropList* createCompressedSetCreatPropList(DataSet* dset);

        static void copyFile(string originalName, string copyName);
        static bool replaceString(string& str, const string& from, const string& to);
        static void unlinkLogs(H5File* file,string path);
        static int stringToInt(string bitString);
    };
}