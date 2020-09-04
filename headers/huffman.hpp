#include <map>
#include <string>
#include <vector>
using namespace std;
namespace huffman {

#define MAX_TREE_HT 100

    typedef struct MinHeapNode {
        int number;
        int freq;
        struct MinHeapNode *left, *right;
    } MinHeapNode;

    typedef struct MinHeap {
        unsigned size;
        unsigned capacity;
        struct MinHeapNode** array;
    } MinHeap;

    class Huffman {
    public:
        static void generateTree(std::map<int,int> readMap);
        static void generateNewTree(std::map<string, int> huffmanMap, std::vector<string> codes, MinHeapNode* node, string code = "");
    };
}

