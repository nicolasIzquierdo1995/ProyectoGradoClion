// Header file:
#include <string>
#include <vector>
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
        float mean;
        float stdv;
    } compressedEventData;

    class Utils {
    public:
        static DataSet* GetDataset(H5File* file, string path, string dataSetGrandParentName, string dataSetName);
        static CompType getEventDataType();
        static CompType getCompressedEventDataType();
        static bool replaceString(string& str, const string& from, const string& to);
        static DSetCreatPropList* createCompressedSetCreatPropList(DataSet* dSet);
        static DSetCreatPropList *createDecompressedSetCreatPropList(int size);
        static PredType getHuffmanSignalDataType();
        static PredType getCompressedSignalDataType();
        static PredType getDecompressedSignalDataType();
        static void copyFile(string originalName, string copyName);
        static void unlinkLogs(H5File* file,string path);
        static vector<DataSet>* listDatasets(string name,H5File* file,string path);
        static int stringToInt(string bitString);
    };
}