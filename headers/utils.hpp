// Header file:
#include <string>
#include <vector>
#include "H5Cpp.h"

using namespace H5;
using namespace std;
namespace utils{

    typedef struct datasetList{
        int size;
        vector<DataSet> ds;
    }datasetList;

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
        static DSetCreatPropList* createCompressedSetCreatPropList(DataSet* dSet);
        static DSetCreatPropList *createDecompressedSetCreatPropList(DataSet* dSet);
        static PredType getIntType(int* buffer, int count);
        static StdvAndMean getStdvAndMean(int* buffer, int start, int length);
        static PredType getHuffmanSignalDataType();
        static PredType getCompressedSignalDataType();
        static PredType getDecompressedSignalDataType();
        static void copyFile(string originalName, string copyName);
        static void unlinkLogs(H5File file,string path);
        static void listDatasets(string name,H5File file,string path,datasetList* result);
        static int stringToInt(string bitString);
    };
}