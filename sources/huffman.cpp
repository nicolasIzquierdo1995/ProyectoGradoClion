#include "../headers/huffman.hpp"
#include <iostream>
#include <cstdlib>

using namespace huffman;
using namespace std;

struct MinHeapNode* newNode(int number, int freq)
{
    struct MinHeapNode* temp
            = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));

    temp->left = temp->right = NULL;
    temp->number = number;
    temp->freq = freq;
    return temp;
}

struct MinHeap* createMinHeap(unsigned capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(minHeap-> capacity * sizeof(struct MinHeapNode*));
    return minHeap;
}

void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) {
    struct MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(struct MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < minHeap->size && minHeap->array[left]->
            freq < minHeap->array[smallest]->freq)
        smallest = left;

    if (right < minHeap->size && minHeap->array[right]->
            freq < minHeap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swapMinHeapNode(&minHeap->array[smallest],
                        &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

int isSizeOne(struct MinHeap* minHeap) {
    return (minHeap->size == 1);
}

struct MinHeapNode* extractMin(struct MinHeap* minHeap) {

    struct MinHeapNode* temp = minHeap->array[0];
    minHeap->array[0]
            = minHeap->array[minHeap->size - 1];

    --minHeap->size;
    minHeapify(minHeap, 0);

    return temp;
}

void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* minHeapNode) {
    ++minHeap->size;
    int i = minHeap->size - 1;
    while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {

        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    minHeap->array[i] = minHeapNode;
}

void buildMinHeap(struct MinHeap* minHeap) {
    int n = minHeap->size - 1;
    int i;

    for (i = (n - 1) / 2; i >= 0; --i)
        minHeapify(minHeap, i);
}

void printArr(int arr[], int n) {
    int i;
    for (i = 0; i < n; ++i)
        cout<< arr[i];

    cout<<"\n";
}

int isLeaf(struct MinHeapNode* root) {
    return !(root->left) && !(root->right);
}

struct MinHeap* createAndBuildMinHeap(std::map<int,int> readMap, int size) {
    map<int,int>::iterator it2;
    struct MinHeap* minHeap = createMinHeap(size);

    int i =0;
    for(it2 = readMap.begin(); it2 != readMap.end(); ++it2) {
        minHeap->array[i] = newNode(it2->first, it2->second);
        i++;
    }

    minHeap->size = size;
    buildMinHeap(minHeap);
    return minHeap;
}

struct MinHeapNode* buildHuffmanTree(std::map<int,int> readMap, int size) {
    struct MinHeapNode *left, *right, *top;
    struct MinHeap* minHeap = createAndBuildMinHeap(readMap, size);

    while (!isSizeOne(minHeap)) {
        left = extractMin(minHeap);
        right = extractMin(minHeap);
        top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;
        insertMinHeap(minHeap, top);
    }
    return extractMin(minHeap);
}

void printCodes(struct MinHeapNode* root, int arr[], int top) {
    if (root->left) {
        arr[top] = 0;
        printCodes(root->left, arr, top + 1);
    }

    if (root->right) {
        arr[top] = 1;
        printCodes(root->right, arr, top + 1);
    }

    if (isLeaf(root)) {
        cout<< root->number <<": ";
        printArr(arr, top);
    }
}

void HuffmanCodes(std::map<int,int> readMap, int size) {
    struct MinHeapNode* root = buildHuffmanTree(readMap, size);
    int arr[MAX_TREE_HT], top = 0;
    printCodes(root, arr, top);
}

void Huffman::generateTree(std::map<int,int> readMap) {
    int size = readMap.size() ;
    HuffmanCodes(readMap, size);
}


bool anyStartsWith(std::vector<string> codes, string code) {
    for (vector<string>::iterator it = codes.begin(); it != codes.end(); ++it){
        if (it->rfind(code) == 2){
            return true;
        }
    }
    return false;
}

void Huffman::generateNewTree(std::map<string, int> huffmanMap, std::vector<string> codes, MinHeapNode* node, string code) {

    string codeLeft = code + "0";
    string codeRight = code + "1";
    bool hasLeftBranch = anyStartsWith(codes, codeLeft);
    bool hasRightBranch = anyStartsWith(codes, codeRight);

    node->freq = 0;
    if (!hasLeftBranch && !hasRightBranch) {
        std::map<string, int>::const_iterator pos = huffmanMap.find(": " + code);
        node->number = pos->second;
        node->left = NULL;
        node->right = NULL;
    }
    else {
        node->number = 666;
        if (hasLeftBranch){
            node->left = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
            generateNewTree(huffmanMap, codes, node->left, codeLeft);
        }
        else {
            node->left = NULL;
        }
        if (hasRightBranch){
            node->right = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
            generateNewTree(huffmanMap, codes, node->right, codeRight);
        }
        else {
            node->right = NULL;
        }
    }
}
