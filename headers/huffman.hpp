

#include <map>
namespace huffman {

// This constant can be avoided by explicitly
// calculating height of Huffman Tree
#define MAX_TREE_HT 100

// A Huffman tree node
    typedef struct MinHeapNode {

        // One of the input characters
        int number;

        // Frequency of the character
        int freq;

        // Left and right child of this node
        struct MinHeapNode *left, *right;
    } MinHeapNode;

// A Min Heap: Collection of
// min-heap (or Huffman tree) nodes
    typedef struct MinHeap {

        // Current size of min heap
        unsigned size;

        // capacity of min heap
        unsigned capacity;

        // Attay of minheap node pointers
        struct MinHeapNode** array;
    } MinHeap;

    class Huffman {
    public:
        static void generateTree(std::map<int,int> readMap);
    };
}

