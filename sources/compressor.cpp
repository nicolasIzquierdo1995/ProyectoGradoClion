#include "../headers/compressor.hpp"
#include "../headers/repack.hpp"
#include "../headers/utils.hpp"
#include "../headers/huffman.hpp"
#include "../headers/inputOutput.hpp"
#include "../headers/errorHandler.hpp"
#include "../headers/stats.hpp"
#include "h5Array.cpp"
#include <map>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <bitset>
#include <chrono>

using namespace std;
using namespace compressor;
using namespace H5;
using namespace utils;
using namespace h5repack;
using namespace huffman;
using namespace inputOutput;
using namespace stats;
using namespace errorHandler;

map<string,int> globalAttributes;
uint16_t* firstReads;
int firstReadsCount;
map<int,string> huffmanMap;
MinHeapNode*huffmanTree = NULL;
bool compressEvents = false;
const int huffmanTokenIndex = -666;


Compressor::Compressor(){

}

// guarda los atributos correspondientes en el archivo
void saveAttributes(string fileName, int compressionLvl){
    if(!globalAttributes.empty()){
        map<string,int>::iterator it = globalAttributes.begin();
        H5File file(fileName, H5F_ACC_RDWR);
        Group root = file.openGroup("/");
        hsize_t dims[1] = { 1 };
        while(it != globalAttributes.end()){
            DataSpace ds = DataSpace(1, dims);
            string attName = it->first;
            int value[1] = { it->second };
            Attribute at = root.createAttribute( attName, Utils::getSignalDataType(),ds);
            at.write( Utils::getSignalDataType(), value);
            it++;
        }

        if(compressionLvl == 3 || compressionLvl  == 4){
            DataSpace dataSpace = Utils::getDataspace(firstReadsCount + 1, firstReadsCount + 1);
            DSetCreatPropList *creatPropList = Utils::createCompressedSetCreatPropList(firstReadsCount + 1);
            DataSet * newSignalsDataset = new DataSet(file.createDataSet("first_reads", Utils::getSignalDataType(), dataSpace, *creatPropList));
            newSignalsDataset->write(firstReads, Utils::getSignalDataType(), dataSpace, dataSpace);
        }
    }
}

MinHeapNode* getHuffmanTreeFromFile() {
    string line;
    map<string,int> huffmanMap;
    vector<string> huffmanCodes;
    string limit = ": ";
    string file = Utils::getHuffmanTreeString();

    std::string result;
    std::istringstream iss(file);

    for (std::string line; std::getline(iss, line); ) {
        int diff = stoi(line.substr(0,line.find(limit)));
        string code = line.substr(line.find(limit));
        huffmanMap[code] = diff;
        huffmanCodes.push_back(code);
    }

    MinHeapNode* tree = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    Huffman::generateNewTree(huffmanMap, huffmanCodes, tree);
    return tree;
}

// descomprime el buffer de huffman en uno de diferencias
vector<int> mapSignalBufferD(h5Array<int16_t> huffmanBuffer){
    if (huffmanTree == NULL){
        huffmanTree = getHuffmanTreeFromFile();
    }
    string bitstring;
    for(int i = 0; i < huffmanBuffer.size; i++){
        bitstring += bitset<16>(huffmanBuffer.ptr[i]).to_string();
    }
    int i = 0;
    int stringSize = bitstring.size();
    vector<int> vec;
    while(i < stringSize){
        MinHeapNode* aux_tree = huffmanTree;
        bool found = false;
        while(!found && i < stringSize){
            char c = bitstring.at(i);
            if(c == '1'){
                aux_tree = aux_tree->right;
            }
            else {
                aux_tree = aux_tree->left;
            }
            if (aux_tree->number != 666){
                found = true;
                int leaf = aux_tree->number;
                if (leaf == huffmanTokenIndex && i+17 < stringSize) {
                    leaf = Utils::stringToInt(bitstring.substr(i + 1, 16));
                    i = i + 16;
                    vec.push_back(leaf);
                }
                else if (leaf != huffmanTokenIndex) {
                    vec.push_back(leaf);
                }
                else {
                    i = i + 64;
                }
            }
            i++;
        }
    }

    return vec;
}


h5Array<DataSet> getDataSetList(H5File* file,string name){
    vector<DataSet>* dataSets = Utils::listDatasets(name,file,"/");
    return h5Array<DataSet>(&(*dataSets)[0], dataSets->size());
}

void generateHuffmanFromExample(H5File* file) {
    map<int,int>::iterator it2;
    map<int,int> readsMap;
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataSet = signalDataSets.ptr[i];
        DataSpace* signalDataSpace = new DataSpace(currentDataSet.getSpace());
        int signalsCount = Utils::getDatasetSize(signalDataSpace);
        uint16_t* signalsBuffer = new uint16_t[signalsCount];
        currentDataSet.read(signalsBuffer,Utils::getSignalDataType(),*signalDataSpace,*signalDataSpace);

        int min = -200;
        int max = 200;
        for (int j = 1; j< signalsCount; j++){
           int diff = signalsBuffer[j] - signalsBuffer[j-1];
           if (diff <= max && diff >= min){
               if (readsMap.find(diff) == readsMap.end()){
                   readsMap[diff] = 1;
               }
               else {
                   readsMap[diff] = readsMap[diff] + 1;
               }
           }
           else {
               if (readsMap.find(huffmanTokenIndex) == readsMap.end()){
                   readsMap[huffmanTokenIndex] = 1;
               }
               else {
                   readsMap[huffmanTokenIndex] = readsMap[huffmanTokenIndex] + 1;
               }
           }
        }
    }

    Huffman::generateTree(readsMap);
}

void generateCsvFromExample(H5File* file) {
    map<int,int>::iterator it2;
    map<int,int> readsMap;
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataSet = signalDataSets.ptr[i];
        DataSpace* signalDataSpace = new DataSpace(currentDataSet.getSpace());
        int signalsCount = Utils::getDatasetSize(signalDataSpace);
        uint16_t* signalsBuffer = new uint16_t[signalsCount];
        currentDataSet.read(signalsBuffer,Utils::getSignalDataType(),*signalDataSpace,*signalDataSpace);

        for (int j = 1; j<  signalsCount; j++) {
            int diff = signalsBuffer[j] - signalsBuffer[j-1];

            if (readsMap.find(diff) == readsMap.end()){
                readsMap[diff] = 1;
            }
            else {
                readsMap[diff] = readsMap[diff] + 1;
            }
        }
    }

    ofstream myFile;
    myFile.open ("../Files/stats.csv");

    for(it2 = readsMap.begin(); it2 != readsMap.end(); ++it2) {
        myFile<< it2->first << "," << it2->second << endl;
    }
    myFile.close();
}

compressedEventData createCompressedEventData(uint8_t skip, uint8_t length, float stdv, float mean) {
    compressedEventData result = { skip, length, mean, stdv };
    return result;
}

// obtiene el buffer de eventos comprimidos
h5Array<compressedEventData> getCompressedEventsBuffer(DataSet *eventsDataset) {

    CompType eventDataType = Utils::getEventDataType();

    DataSpace* eventDataSpace = new DataSpace(eventsDataset->getSpace());
    int eventsCount = Utils::getDatasetSize(eventDataSpace);
    eventData* eventsBuffer = new eventData[eventsCount];

    compressedEventData* compressedEventBuffer = new compressedEventData [eventsCount];;

    eventsDataset->read(eventsBuffer,eventDataType,*eventDataSpace,*eventDataSpace);
    globalAttributes.insert(pair<string,int>(eventsDataset->getObjName() + "_firstEvent",eventsBuffer[0].start));

    for(int i = 0; i< eventsCount - 1; i++){
        uint8_t skip = eventsBuffer[i + 1].start - (eventsBuffer[i].start + eventsBuffer[i].length);
        createCompressedEventData(skip, eventsBuffer[i].length, eventsBuffer[i].mean, eventsBuffer[i].stdv);
    }

    createCompressedEventData(0, eventsBuffer[eventsCount-1].length, eventsBuffer[eventsCount-1].mean, eventsBuffer[eventsCount-1].stdv);
    return h5Array<compressedEventData>(compressedEventBuffer,eventsCount);
}

// obtiene el buffer de eventos descomprimidos
h5Array<eventData> getDecompressedEventsBuffer(H5File* file, DataSet *compressedEventsDataset) {

    CompType eventsDataType = Utils::getCompressedEventDataType();

    DataSpace* eventDataSpace = new DataSpace(compressedEventsDataset->getSpace());
    int eventsCount = Utils::getDatasetSize(eventDataSpace);
    eventData* eventsBuffer = new eventData[eventsCount];
    compressedEventData* compressedEventBuffer = new compressedEventData [eventsCount];
    compressedEventsDataset->read(compressedEventBuffer,eventsDataType,*eventDataSpace,*eventDataSpace);

    int start;
    int readStart = 0;
    string firstEventAttributeName = compressedEventsDataset->getObjName() + "_firstEvent";
    Group root = file->openGroup("/");
    Attribute at = root.openAttribute(firstEventAttributeName);
    DataType dt = at.getDataType();
    at.read(dt,&start);

    H5Adelete(root.getId(),firstEventAttributeName.c_str());

    for(int i = 0; i < eventsCount; i++){
        int length = compressedEventBuffer[i].length;
        eventsBuffer[i].length  = length;
        eventsBuffer[i].start = start;
        eventsBuffer[i].mean  = compressedEventBuffer[i].mean;
        eventsBuffer[i].stdv = compressedEventBuffer[i].stdv;
        start = start + length + compressedEventBuffer[i].skip;
        readStart = readStart + length + compressedEventBuffer[i].skip;
    }
    return h5Array<eventData>(eventsBuffer,eventsCount);
}


// obtiene el buffer de signals comprimidos
h5Array<int16_t> getCompressedSignalBuffer(DataSet *signalDataset, int index) {
    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    int signalsCount = Utils::getDatasetSize(signalDataSpace);
    uint16_t* signalsBuffer = new uint16_t[signalsCount];
    int16_t* compressedSignalsBuffer = new int16_t[signalsCount];
    signalDataset->read(signalsBuffer,Utils::getSignalDataType(),*signalDataSpace,*signalDataSpace);

    firstReads[index] = signalsBuffer[0];
    firstReadsCount = index;
    compressedSignalsBuffer[0] = 0;
    for(int i = 1; i< signalsCount; i++){
        compressedSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }
    return h5Array<int16_t>(compressedSignalsBuffer,signalsCount);
}

// descomprime el buffer de signal
h5Array<uint16_t> getDecompressedSignalBuffer(H5File* file, DataSet *signalDataset,bool useHuffman, uint16_t firstRead) {
    Group root = file->openGroup("/");
    DataSpace signalDataSpace = signalDataset->getSpace();
    int signalsCount = Utils::getDatasetSize(signalDataset);
    int decompressedCount;
    vector<int> signalsBuffer;

    if (useHuffman){
        int16_t* huffmanBuffer = new int16_t[signalsCount];
        signalDataset->read(huffmanBuffer,Utils::getCompressedSignalDataType(),signalDataSpace,signalDataSpace);
        signalsBuffer = mapSignalBufferD(h5Array<int16_t>(huffmanBuffer,signalsCount));
        decompressedCount = signalsBuffer.size();
    }
    else {
        int16_t* signalsBufferAux = new int16_t[signalsCount];
        signalDataset->read(signalsBufferAux,Utils::getCompressedSignalDataType(),signalDataSpace,signalDataSpace);
        decompressedCount = signalsCount;
        vector<int> aux(signalsBufferAux, signalsBufferAux + signalsCount);
        signalsBuffer = aux;
    }

    uint16_t* newSignalsBuffer = new uint16_t[decompressedCount];
    newSignalsBuffer[0] = firstRead;
    for(int i = 1; i < decompressedCount; i++){
        newSignalsBuffer[i] = newSignalsBuffer[i-1] + signalsBuffer[i];
    }

    return h5Array<uint16_t>(newSignalsBuffer,decompressedCount);
}

// mapea el buffer de diferencias a un buffer de huffman
h5Array<int16_t> mapSignalBufferC(h5Array<int16_t> buffer) {
    string bitstring;
    string huffmanValue;
    string huffmanToken = huffmanMap.at(huffmanTokenIndex);

    for (int i = 0; i < buffer.size; i++){
        int16_t currentValue = buffer.ptr[i];
        if (huffmanMap.count(currentValue) > 0) { // huffmanMap.exists(currentValue)
            huffmanValue = huffmanMap.at(currentValue);
        }
        else {
            huffmanValue = huffmanToken + bitset<16>(currentValue).to_string();
        }
        bitstring.append(huffmanValue);
    }

    bitstring.append(huffmanToken);

    int position = 0;
    int16_t currentInt = 0;
    int count = (int)bitstring.length() / 16 +  (bitstring.length() % 16 != 0 ? 1 : 0);
    int16_t * intArray = new int16_t[count]();

    int i = 0;
    for (char const &c: bitstring){
        if (c == '1'){
            currentInt |= 1 << 15 - position;
        }
        if (position < 15){
            position++;
        }
        else{
            intArray[i] = currentInt;
            currentInt = 0;
            position = 0;
            i++;
        }
    }

    if (bitstring.length() % 16 != 0){
        intArray[i] = currentInt;
    }

    return h5Array<int16_t>(intArray, count);
}


//Comprime usando szip
void szipCompression(H5File* file, string newFileName) {
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    string* signalDatasetNames = new string[signalDataSets.size];
    h5Array<uint16_t>* compressedSignalsBuffers = new h5Array<uint16_t>[signalDataSets.size];

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataset = signalDataSets.ptr[i];
        DataSpace* currentDataSpace = new DataSpace (currentDataset.getSpace());
        int signalsCount = Utils::getDatasetSize(&currentDataset);
        uint16_t* signalsBuffer = new uint16_t[signalsCount];
        currentDataset.read(signalsBuffer,Utils::getSignalDataType(),*currentDataSpace,*currentDataSpace);

        signalDatasetNames[i] = currentDataset.getObjName();
        compressedSignalsBuffers[i] = h5Array<uint16_t>(signalsBuffer,signalsCount);

        file->unlink(currentDataset.getObjName().c_str());
    }

    h5repack::repack(file, newFileName, "9");
    H5File newFile(newFileName, H5F_ACC_RDWR);

    for (int i = 0; i < signalDataSets.size; i++){
        int size = compressedSignalsBuffers[i].size;
        DataSpace dataSpace = Utils::getDataspace(size, size);
        DSetCreatPropList* creatPropList = Utils::createCompressedSetCreatPropList(size);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], Utils::getSignalDataType(), dataSpace, *creatPropList));
        newSignalsDataset->write(compressedSignalsBuffers[i].ptr, Utils::getSignalDataType(), dataSpace, dataSpace);
    }
}


// comprime eventos y reads de un archivo
void compressEventsAndReads(H5File* file,string newFileName, bool useHuffman){
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    firstReads = new uint16_t[signalDataSets.size];
    h5Array<int16_t>* compressedSignalBuffers = new h5Array<int16_t>[signalDataSets.size];
    string* signalDatasetNames = new string[signalDataSets.size];
    h5Array<DataSet> eventDataSets;
    string* eventsDatasetNames;
    DataSpace** eventsDataSpaces;
    h5Array<compressedEventData>* compressedEventsBuffers;

    if (compressEvents){
        eventDataSets = getDataSetList(file,"Events");
        compressedEventsBuffers = new h5Array<compressedEventData>[eventDataSets.size];
        string* eventsDatasetNames = new string[eventDataSets.size];
        eventsDataSpaces = new DataSpace*[eventDataSets.size];

        for (int i = 0; i < eventDataSets.size; i++){
            DataSet currentDataset = eventDataSets.ptr[i];
            eventsDatasetNames[i] = currentDataset.getObjName();
            eventsDataSpaces[i] = new DataSpace(currentDataset.getSpace());
            compressedEventsBuffers[i] = getCompressedEventsBuffer(&currentDataset);
            file->unlink(currentDataset.getObjName().c_str());
        }
    }

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataset = signalDataSets.ptr[i];
        signalDatasetNames[i] = currentDataset.getObjName();
        compressedSignalBuffers[i] = getCompressedSignalBuffer(&currentDataset, i);
        file->unlink(currentDataset.getObjName().c_str());
    }

    h5repack::repack(file, newFileName, "9");
    H5File newFile(newFileName, H5F_ACC_RDWR);
    CompType compressedEventDataType = Utils::getCompressedEventDataType();;

    if (compressEvents) {
        for (int i = 0; i < eventDataSets.size; i++){
            DSetCreatPropList *eventsPlist = Utils::createCompressedSetCreatPropList(&eventDataSets.ptr[i]);
            DataSet *newEventsDataset = new DataSet(
                    newFile.createDataSet(eventsDatasetNames[i], compressedEventDataType, *eventsDataSpaces[i], *eventsPlist));
            newEventsDataset->write(compressedEventsBuffers[i].ptr, compressedEventDataType);
        }
    }

    PredType compressedSignalDataType = Utils::getCompressedSignalDataType();
    if (useHuffman) {
        for (int i = 0; i < signalDataSets.size; i++){
            h5Array<int16_t> huffmanSignalBuffer = mapSignalBufferC(compressedSignalBuffers[i]);
            DataSpace dataSpace = Utils::getDataspace(huffmanSignalBuffer.size, huffmanSignalBuffer.size);

            DataSet newSignalsDataset = DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, dataSpace));
            newSignalsDataset.write(huffmanSignalBuffer.ptr, compressedSignalDataType, dataSpace);
        }
    }
    else {
        for (int i = 0; i < signalDataSets.size; i++){
            int size = compressedSignalBuffers[i].size;
            DataSpace dataSpace = Utils::getDataspace(size, size);
            DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, dataSpace));
            newSignalsDataset->write(compressedSignalBuffers[i].ptr, compressedSignalDataType, dataSpace, dataSpace);
        }
    }
}

//deja todos los reads con szip en el grupo principal
void getOnlyReads(H5File* file,string newFileName){
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    PredType signalDataType = Utils::getSignalDataType();
    H5File newFile(newFileName, H5F_ACC_TRUNC);

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataset = signalDataSets.ptr[i];
        DataSpace signalDataSpace = currentDataset.getSpace();
        string currentDatasetName = "/" + Utils::removeChar(currentDataset.getObjName(),'/');
        int signalsCount = Utils::getDatasetSize(&currentDataset);
        int* compressedSignalBuffer = new int[signalsCount];
        DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(signalsCount);

        currentDataset.read(compressedSignalBuffer,signalDataType,signalDataSpace);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(currentDatasetName, signalDataType, signalDataSpace, *readsPList));
        newSignalsDataset->write(compressedSignalBuffer, signalDataType, signalDataSpace, signalDataSpace);
    }
}

// descomprime los reads y los eventos
void deCompressEventsAndReads(H5File* file,string newFileName, bool useHuffman){

    h5Array<DataSet> eventDataSets;
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");

    h5Array<eventData>* decompressedEventsBuffers;
    string* eventsDatasetNames;
    DataSpace** eventsDataSpaces;

    DataSet firstReadsDataset = file->openDataSet("/first_reads");
    DataSpace firstReadsDataSpace = firstReadsDataset.getSpace();
    uint16_t * firstReadsBuffer = new uint16_t[signalDataSets.size];
    firstReadsDataset.read(firstReadsBuffer,Utils::getSignalDataType(),firstReadsDataSpace,firstReadsDataSpace);

    if (compressEvents) {
        eventDataSets = getDataSetList(file,"Events");
        decompressedEventsBuffers = new h5Array<eventData>[eventDataSets.size];
        eventsDatasetNames = new string[eventDataSets.size];
        eventsDataSpaces = new DataSpace*[eventDataSets.size];

        for (int i = 0; i < eventDataSets.size; i++){
            DataSet currentDataSet = eventDataSets.ptr[i];
            decompressedEventsBuffers[i] = getDecompressedEventsBuffer(file, &currentDataSet);
            eventsDatasetNames[i] = currentDataSet.getObjName();
            eventsDataSpaces[i] = new DataSpace(currentDataSet.getSpace());
            file->unlink(currentDataSet.getObjName().c_str());
        }
    }

    h5Array<uint16_t>* decompressedSignalBuffers = new h5Array<uint16_t>[signalDataSets.size];
    string* signalDatasetNames = new string[signalDataSets.size];
    DataSpace* signalDataSpaces = new DataSpace[signalDataSets.size];

    for (int i = 0; i < signalDataSets.size; i++) {
        DataSet currentDataSet = signalDataSets.ptr[i];
        signalDatasetNames[i] = currentDataSet.getObjName();
        decompressedSignalBuffers[i] = getDecompressedSignalBuffer(file, &currentDataSet,useHuffman, firstReadsBuffer[i]);
        signalDataSpaces[i] = Utils::getDataspace(decompressedSignalBuffers[i].size, H5S_UNLIMITED);

        file->unlink(currentDataSet.getObjName().c_str());
    }

    file->unlink(firstReadsDataset.getObjName().c_str());

    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);
    PredType decompressedSignalDataType = Utils::getSignalDataType();

    if (compressEvents) {
        CompType eventDataType = Utils::getEventDataType();
        for (int i = 0; i < eventDataSets.size; i++){
            DataSet currentDataSet = eventDataSets.ptr[i];
            int size = Utils::getDatasetSize(&currentDataSet);
            DSetCreatPropList* eventsPlist =  Utils::createDecompressedSetCreatPropList(size);
            DataSet* newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetNames[i], eventDataType, *eventsDataSpaces[i], *eventsPlist));
            newEventsDataset->write(decompressedEventsBuffers[i].ptr, eventDataType);
        }
    }

    for (int i = 0; i < signalDataSets.size; i++){
        DSetCreatPropList* readsPList = Utils::createDecompressedSetCreatPropList(decompressedSignalBuffers[i].size);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], decompressedSignalDataType, signalDataSpaces[i], *readsPList));
        newSignalsDataset->write(decompressedSignalBuffers[i].ptr, decompressedSignalDataType, signalDataSpaces[i], signalDataSpaces[i]);
    }

}

void removeLogs(H5File* file,string newFileName) {
    Utils::unlinkLogs(file,"/");
    h5repack::repack(file, newFileName, "9");
}

// lee el archivo del arbol de huffman
void readTreeFile() {
    string line;
    int pos;
    string limit = ": ";
    string file = Utils::getHuffmanTreeString();

    std::string result;
    std::istringstream iss(file);

    for (std::string line; std::getline(iss, line); ) {
        string sPos = line.substr(0,line.find(limit));
        string sVal = line.substr(line.find(limit) + 2);
        pos = stoi(sPos);
        huffmanMap.insert(std::pair<int,string>(pos,sVal));
    }
}

//FUNCION DE COMPRESION
void Compressor::CompressFile(H5File* file, int compressionLevel){

    InputOutput::PrintOutput("Compression Level: " + std::to_string(compressionLevel));
    string compressedFileName = file->getFileName();
    Utils::replaceString(compressedFileName, "_copy.fast5", "_repacked.fast5");

    //guarda nivel de compresion en atributo
    if (compressionLevel > 0 && compressionLevel < 6){
        globalAttributes.insert(pair<string,int>("compLevel",compressionLevel));
    }
    try {
        switch(compressionLevel) {
            case (0):
                getOnlyReads(file,compressedFileName);
                break;
            case (1):
                h5repack::repack(file, compressedFileName, "9");
                break;
            case (2):
                szipCompression(file,compressedFileName);
                break;
            case (3):
            case (4):
                readTreeFile();
                compressEventsAndReads(file,compressedFileName,compressionLevel == 4);
                break;
            case (5):
                removeLogs(file, compressedFileName);
                break;
            case (6):
                Stats::getStats(file, true);
                break;
            case (7):
                Stats::getStats(file,false);
                break;
            case (8):
                Stats::getStats(file);
                break;
            case (9):
                generateHuffmanFromExample(file);
                break;
            case (10):
                generateCsvFromExample(file);
                break;
            default:
                ErrorHandler::handleError(3);
        }
    }
    catch(Exception ex){
        ErrorHandler::handleError(4);
    }


    saveAttributes(compressedFileName,compressionLevel);
}

void Compressor::DeCompressFile(H5File* file, int compressionLevel){
    InputOutput::PrintOutput("Decompression Level: " + std::to_string(compressionLevel));
    string fileName = file->getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, "_copy.fast5", "_deCompressed.fast5");

    string attrName = "compLevel";;
    Group root = file->openGroup("/");
    H5Adelete(root.getId(),attrName.c_str());

    try {
        if (compressionLevel == 1 || compressionLevel == 2) {
            h5repack::repack(file, deCompressedFileName, "3");
        } else if (compressionLevel == 3 || compressionLevel == 4) {
            deCompressEventsAndReads(file,deCompressedFileName,compressionLevel == 4);
        }
        else {
            ErrorHandler::handleError(3);
        }
    }
    catch(Exception ex){
        ErrorHandler::handleError(5);
    }
}







