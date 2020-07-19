#include "../headers/compresser.hpp"
#include "../headers/repack.hpp"
#include "../headers/utils.hpp"
#include "../headers/huffman.hpp"
#include "h5Array.cpp"
#include <map>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <bitset>
#include <chrono>

using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
using namespace h5repack;
using namespace huffman;

map<string,int> globalAttributes;
uint16_t* firstReads;
int firstReadsCount;
map<int,string> treeC;
MinHeapNode*huffmanTree = NULL;
bool compressEvents = false;


Compresser::Compresser(){

}

void saveAtributes(string fileName){
    if(!globalAttributes.empty()){
        map<string,int>::iterator it = globalAttributes.begin();
        H5File file(fileName, H5F_ACC_RDWR);
        Group root = file.openGroup("/");
        hsize_t dims[1] = { 1 };
        while(it != globalAttributes.end()){
            DataSpace ds = DataSpace(1, dims);
            string attName = it->first;
            int value[1] = { it->second };
            Attribute at = root.createAttribute( attName, PredType::NATIVE_INT,ds);
            at.write( PredType::NATIVE_INT, value);
            it++;
        }

        hsize_t chunk_dims[1] = { (hsize_t)firstReadsCount + 1};
        DataSpace dataSpace =  DataSpace(1, chunk_dims, chunk_dims);
        DSetCreatPropList *creatPropList = Utils::createCompressedSetCreatPropList(firstReadsCount + 1);
        DataSet * newSignalsDataset = new DataSet(file.createDataSet("first_reads", PredType::NATIVE_UINT16, dataSpace, *creatPropList));
        newSignalsDataset->write(firstReads, PredType::NATIVE_UINT16, dataSpace, dataSpace);
    }
}

MinHeapNode* getHuffmanTreeFromFile(string fileName) {
    ifstream inFile;
    string line;
    map<string,int> huffmanMap;
    vector<string> huffmanCodes;
    string limit = ": ";
    inFile.open("../Files/" + fileName);

    while(getline(inFile,line)){
        int diff = stoi(line.substr(0,line.find(limit)));
        string code = line.substr(line.find(limit));
        huffmanMap[code] = diff;
        huffmanCodes.push_back(code);
    }
    MinHeapNode* tree = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    Huffman::generateNewTree(huffmanMap, huffmanCodes, tree);
    return tree;
}

vector<int> mapSignalBufferD(h5Array<int16_t> pChar){
    if (huffmanTree == NULL){
        huffmanTree = getHuffmanTreeFromFile("ganamo.txt");
    }
    string bitstring;
    for(int i = 0;i<pChar.size;i++){
        bitstring += bitset<16>(pChar.ptr[i]).to_string();
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
            }else{
                aux_tree = aux_tree->left;
            }
            if(aux_tree->number != 666){
                found = true;
                int leaf = aux_tree->number;
                if (leaf == 201 && i+17 < stringSize) {
                    leaf = Utils::stringToInt(bitstring.substr(i + 1, 16));
                    i = i + 16;
                    vec.push_back(leaf);
                }
                else if (leaf != 201) {
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

void generateHuffmanFromExample(H5File* file){

    map<int,int>::iterator it2;
    map<int,int> readsMap;

    vector<DataSet>* signalDataSets = Utils::listDatasets("Signal",file,"/");
    string* signalDatasetNames = new string[signalDataSets->size()];
    DataSpace** signalDataSpaces = new DataSpace*[signalDataSets->size()];

    int i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->begin(); it != signalDataSets->end(); ++it){
        signalDatasetNames[i] = (*it).getObjName();
        signalDataSpaces[i] = new DataSpace((*it).getSpace());

        DataSpace* signalDataSpace = new DataSpace((&*it)->getSpace());
        hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
        signalDataSpace->getSimpleExtentDims(signalDims);
        int signalsCount = (int)(signalDims[0]);
        uint16_t * signalsBuffer = new uint16_t[signalsCount];
        (&*it)->read(signalsBuffer,Utils::getDecompressedSignalDataType(),*signalDataSpace,*signalDataSpace);

        int min = -150;
        int max = 200;
        int firstSignal = signalsBuffer[0];
        for(int j = 1; j< signalsCount; j++){
           int diff = signalsBuffer[j] - firstSignal;
           if (diff <= max && diff >= min){
               if (readsMap.find(diff) == readsMap.end()){
                   readsMap[diff] = 1;
               }
               else {
                   readsMap[diff] = readsMap[diff] + 1;
               }
           }
           else {
               if (readsMap.find(666) == readsMap.end()){
                   readsMap[666] = 1;
               }
               else {
                   readsMap[666] = readsMap[666] + 1;
               }
           }
           //int diff = signalsBuffer[j] - firstSignal;
           //if (readsMap.find(diff) == readsMap.end()){
           //    readsMap[diff] = 1;
           //}
           //else {
           //    readsMap[diff] = readsMap[diff] + 1;
           //}
        }

        i++;
    }

    //Huffman::generateTree(readsMap);
    ofstream myfile;
    myfile.open ("archivo2.csv");

    for(it2 = readsMap.begin(); it2 != readsMap.end(); ++it2) {
        myfile<< it2->first << "," << it2->second << endl;
    }
    myfile.close();
}

h5Array<compressedEventData> getCompressedEventsBuffer(DataSet *eventsDataset) {

    CompType originalEventDT = Utils::getEventDataType();

    DataSpace* originalEventDS = new DataSpace(eventsDataset->getSpace());
    hsize_t originalEventsD[originalEventDS->getSimpleExtentNdims()];
    originalEventDS->getSimpleExtentDims(originalEventsD);
    unsigned long eventsCount = (unsigned long)(originalEventsD[0]);
    eventData* originalEventsBuffer = new eventData[eventsCount];

    compressedEventData* eventBuffer = new compressedEventData [eventsCount];;

    eventsDataset->read(originalEventsBuffer,originalEventDT,*originalEventDS,*originalEventDS);
    globalAttributes.insert(pair<string,int>(eventsDataset->getObjName() + "_firstEvent",originalEventsBuffer[0].start));

    for(int i = 0; i< eventsCount - 1; i++){
        eventBuffer[i].skip  = originalEventsBuffer[i + 1].start - (originalEventsBuffer[i].start + originalEventsBuffer[i].length);
        eventBuffer[i].length = originalEventsBuffer[i].length;
        eventBuffer[i].stdv = originalEventsBuffer[i].stdv;
        eventBuffer[i].mean = originalEventsBuffer[i].mean;
    }

    eventBuffer[eventsCount-1].skip = 0;
    eventBuffer[eventsCount-1].length = originalEventsBuffer[eventsCount-1].length;
    eventBuffer[eventsCount-1].stdv = originalEventsBuffer[eventsCount-1].stdv;
    eventBuffer[eventsCount-1].mean = originalEventsBuffer[eventsCount-1].mean;

    h5Array<compressedEventData> ret = h5Array<compressedEventData>(eventBuffer,eventsCount);
    return ret;
}

h5Array<eventData> getDecompressedEventsBuffer(H5File* file, DataSet *compressedEventsDataset) {

    CompType compressedEventDT = Utils::getCompressedEventDataType();

    DataSpace* compressedEventDS = new DataSpace(compressedEventsDataset->getSpace());
    hsize_t compressedEventsD[compressedEventDS->getSimpleExtentNdims()];
    compressedEventDS->getSimpleExtentDims(compressedEventsD);
    unsigned long eventsCount = (unsigned long)(compressedEventsD[0]);
    eventData* decompressedEventsBuffer = new eventData[eventsCount];
    compressedEventData* compressedEventBuffer = new compressedEventData [eventsCount];;

    compressedEventsDataset->read(compressedEventBuffer,compressedEventDT,*compressedEventDS,*compressedEventDS);

    int start;
    int readStart = 0;
    string firstEventAttributeName = compressedEventsDataset->getObjName() + "_firstEvent";
    Group root = file->openGroup("/");
    Attribute at = root.openAttribute(firstEventAttributeName);
    DataType dt = at.getDataType();
    at.read(dt,&start);

    H5Adelete(root.getId(),firstEventAttributeName.c_str());

    for(int i = 0; i< eventsCount; i++){
        int length = compressedEventBuffer[i].length;
        decompressedEventsBuffer[i].length  = length;
        decompressedEventsBuffer[i].start = start;
        decompressedEventsBuffer[i].mean  = compressedEventBuffer[i].mean;
        decompressedEventsBuffer[i].stdv = compressedEventBuffer[i].stdv;
        start = start + length + compressedEventBuffer[i].skip;
        readStart = readStart + length + compressedEventBuffer[i].skip;
    }
    h5Array<eventData> ret = h5Array<eventData>(decompressedEventsBuffer,eventsCount);
    return ret;
}

h5Array<int16_t> getCompressedSignalBuffer(DataSet *signalDataset, int index) {

    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
    signalDataSpace->getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    uint16_t* signalsBuffer = new uint16_t[signalsCount];

    int16_t* newSignalsBuffer = new int16_t[signalsCount];
    signalDataset->read(signalsBuffer,Utils::getDecompressedSignalDataType(),*signalDataSpace,*signalDataSpace);

    firstReads[index] = signalsBuffer[0];
    firstReadsCount = index;

    newSignalsBuffer[0] = 0;
    for(int i = 1; i< signalsCount; i++){
        newSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }
    h5Array<int16_t> ret = h5Array<int16_t>(newSignalsBuffer,signalsCount);
    return ret;
}

h5Array<uint16_t> getDecompressedSignalBuffer(H5File* file, DataSet *signalDataset,int compressionLevel, uint16_t firstRead) {

    //chrono::steady_clock::time_point begin;
    //chrono::steady_clock::time_point end;

    Group root = file->openGroup("/");
    DataSpace signalDataSpace = signalDataset->getSpace();
    hsize_t signalDims[signalDataSpace.getSimpleExtentNdims()];
    signalDataSpace.getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    int realCount;
    vector<int> signalsBuffer;

    switch(compressionLevel){
        case 2: {
            int* signalsBufferAux = new int[signalsCount];
            signalDataset->read(signalsBufferAux,PredType::NATIVE_INT,signalDataSpace,signalDataSpace);
            realCount = signalsCount;
            vector<int> aux(signalsBufferAux, signalsBufferAux + signalsCount);
            signalsBuffer = aux;
            break;
        }
        case 3:{
            int16_t* huffmanBuffer = new int16_t[signalsCount];
            signalDataset->read(huffmanBuffer,Utils::getHuffmanSignalDataType(),signalDataSpace,signalDataSpace);
            signalsBuffer = mapSignalBufferD(h5Array<int16_t>(huffmanBuffer,signalsCount));
            realCount = signalsBuffer.size();
            break;
        }
        default: exit(1);
    }

    uint16_t* newSignalsBuffer = new uint16_t[realCount];
    newSignalsBuffer[0] = firstRead;
    for(int i = 1; i< realCount; i++){
        newSignalsBuffer[i] = newSignalsBuffer[i-1] + signalsBuffer[i];
    }
    h5Array<uint16_t> ret = h5Array<uint16_t>(newSignalsBuffer,realCount);
    return ret;
}

h5Array<int16_t> mapSignalBufferC(h5Array<int16_t> pInt) {
    string bitstring;
    string aux;

    for (int j = 0; j < pInt.size; j++){
        if(treeC.count(pInt.ptr[j]) > 0) {
            aux = treeC.at(pInt.ptr[j]);
        }else{
            aux = treeC.at(251) + bitset<16>(pInt.ptr[j]).to_string();
        }
        bitstring.append(aux);
    }

    bitstring.append(treeC.at(251));

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

    h5Array<int16_t> ret = h5Array<int16_t>(intArray,count);
    return ret;
}

void unlink(H5File* file, string groupName) {
    file->unlink(groupName);
}

h5Array<DataSet> getDataSetList(H5File* file,string name){
    vector<DataSet>* dataSets = Utils::listDatasets(name,file,"/");
    return h5Array<DataSet>(&(*dataSets)[0], dataSets->size());
}

void printChunks(H5File* file){
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    for (int i = 0; i < signalDataSets.size; i++){
        DataSet ds = signalDataSets.ptr[i];
        DSetCreatPropList propertyList = ds.getCreatePlist();
        DataSpace space = ds.getSpace();
        hsize_t ndims[1];
        space.getSimpleExtentDims(ndims);
        hsize_t chunk_dims[1];
        propertyList.getChunk(1, chunk_dims);
        unsigned int flags;
        size_t cd_nelmts = 1;
        unsigned int* cd_values = new unsigned  int[1];
        size_t namelen;
        char* name = new char[20];
        unsigned int filter_config;
        int numFiltros = propertyList.getNfilters();
        propertyList.getFilter(0, flags, cd_nelmts, cd_values, namelen, name, filter_config);
        cout << "dataset: " << ds.getObjName() << " Numero de filtros: " << numFiltros << " Cantidad de elementos: " << ndims[0] << " Chunks:  " << chunk_dims[0] << endl << " Deflate: " <<  name;
    }

}

void compressEventsAndReads(H5File* file,string newFileName,int compLvl){

    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    firstReads = new uint16_t[signalDataSets.size];
    // Events
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
            unlink(file, currentDataset.getObjName().c_str());
        }
    }
    // End events

    h5Array<int16_t>* compressedSignalBuffers = new h5Array<int16_t>[signalDataSets.size];
    string* signalDatasetNames = new string[signalDataSets.size];


    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataset = signalDataSets.ptr[i];
        signalDatasetNames[i] = currentDataset.getObjName();
        compressedSignalBuffers[i] = getCompressedSignalBuffer(&currentDataset, i);
        unlink(file, currentDataset.getObjName().c_str());
    }

    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();;

    // Events
    if (compressEvents) {
        for (int i = 0; i < eventDataSets.size; i++){
            DSetCreatPropList *eventsPlist = Utils::createCompressedSetCreatPropList(&eventDataSets.ptr[i]);
            DataSet *newEventsDataset = new DataSet(
                    newFile.createDataSet(eventsDatasetNames[i], compressedEventDataType, *eventsDataSpaces[i], *eventsPlist));
            newEventsDataset->write(compressedEventsBuffers[i].ptr, compressedEventDataType);
        }
    }
    // End events

    switch (compLvl) {
        case 2:{
            PredType compressedSignalDataType = Utils::getCompressedSignalDataType();
            for (int i = 0; i < signalDataSets.size; i++){
                hsize_t chunk_dims[1] = { (hsize_t)compressedSignalBuffers[i].size };
                DataSpace dataSpace =  DataSpace(1, chunk_dims, chunk_dims);
                DSetCreatPropList* creatPropList = Utils::createCompressedSetCreatPropList(compressedSignalBuffers[i].size);
                DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, dataSpace, *creatPropList));
                newSignalsDataset->write(compressedSignalBuffers[i].ptr, compressedSignalDataType, dataSpace, dataSpace);
            }
            break;
        }

        case 3:{
            PredType compressedSignalDataType = Utils::getHuffmanSignalDataType();
            for (int i = 0; i < signalDataSets.size; i++){
                h5Array<int16_t> huffmanSignalBuffer = mapSignalBufferC(compressedSignalBuffers[i]);

                hsize_t chunk_dims[1] = { (hsize_t)huffmanSignalBuffer.size };
                DataSpace dataSpace =  DataSpace(1, chunk_dims, chunk_dims);

                DSetCreatPropList* creatPropList = Utils::createCompressedSetCreatPropList(huffmanSignalBuffer.size);
                DataSet newSignalsDataset = DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, dataSpace, *creatPropList));
                newSignalsDataset.write(huffmanSignalBuffer.ptr, compressedSignalDataType, dataSpace);
            }
            break;
        }
        default: exit(1);
    }

}

void getOnlyReads(H5File* file,string newFileName){
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");
    string* signalDatasetNames = new string[signalDataSets.size];
    h5Array<int16_t>* compressedSignalBuffers = new h5Array<int16_t>[signalDataSets.size];
    PredType compressedSignalDataType = Utils::getCompressedSignalDataType();
    H5File newFile(newFileName, H5F_ACC_TRUNC);

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet currentDataset = signalDataSets.ptr[i];
        signalDatasetNames[i] = currentDataset.getObjName();
        DataSpace signalsDataSpace = currentDataset.getSpace();
        hsize_t chunk_dims[1] = { (hsize_t)compressedSignalBuffers->size };
        signalsDataSpace.getSimpleExtentDims(chunk_dims);
        DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(chunk_dims[0]);
        currentDataset.read(compressedSignalBuffers[i].ptr,Utils::getDecompressedSignalDataType(),signalsDataSpace);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, signalsDataSpace, *readsPList));
        newSignalsDataset->write(compressedSignalBuffers[i].ptr, compressedSignalDataType, signalsDataSpace, signalsDataSpace);
    }
}

void deCompressEventsAndReads(H5File* file,string newFileName,int compressionLevel){

    h5Array<DataSet> eventDataSets;
    h5Array<DataSet> signalDataSets = getDataSetList(file,"Signal");

    h5Array<eventData>* decompressedEventsBuffers;
    string* eventsDatasetNames;
    DataSpace** eventsDataSpaces;

    DataSet firstReadsDataset = file->openDataSet("/first_reads");
    DataSpace firstReadsDataSpace = firstReadsDataset.getSpace();
    uint16_t * firstReadsBuffer = new uint16_t[signalDataSets.size];
    firstReadsDataset.read(firstReadsBuffer,PredType::NATIVE_UINT16,firstReadsDataSpace,firstReadsDataSpace);

    // Events
    if (compressEvents) {
        signalDataSets = getDataSetList(file,"Events");
        decompressedEventsBuffers = new h5Array<eventData>[eventDataSets.size];
        eventsDatasetNames = new string[eventDataSets.size];
        eventsDataSpaces = new DataSpace*[eventDataSets.size];

        for (int i = 0; i < eventDataSets.size; i++){
            DataSet curentDataSet = eventDataSets.ptr[i];
            decompressedEventsBuffers[i] = getDecompressedEventsBuffer(file, &curentDataSet);
            eventsDatasetNames[i] = curentDataSet.getObjName();
            eventsDataSpaces[i] = new DataSpace(curentDataSet.getSpace());
            unlink(file, curentDataSet.getObjName().c_str());
        }
    }
    // End events

    h5Array<uint16_t>* decompressedSignalBuffers = new h5Array<uint16_t>[signalDataSets.size];
    string* signalDatasetNames = new string[signalDataSets.size];
    DataSpace** signalDataSpaces = new DataSpace*[signalDataSets.size];

    for (int i = 0; i < signalDataSets.size; i++){

        DataSet curentDataSet = signalDataSets.ptr[i];
        signalDatasetNames[i] = curentDataSet.getObjName();
        decompressedSignalBuffers[i] = getDecompressedSignalBuffer(file, &curentDataSet,compressionLevel, firstReadsBuffer[i]);

        hsize_t chunk_dims[1] = {(hsize_t) decompressedSignalBuffers[i].size};
        hsize_t chunk_dimsUnlimited[1] = { H5S_UNLIMITED };
        signalDataSpaces[i] = new DataSpace(1,chunk_dims, chunk_dimsUnlimited);

        unlink(file, curentDataSet.getObjName().c_str());
    }

    unlink(file, firstReadsDataset.getObjName().c_str());

    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);
    PredType decompressedSignalDataType = Utils::getDecompressedSignalDataType();

    // Events
    if (compressEvents) {
        CompType eventDataType = Utils::getEventDataType();
        for (int i = 0; i < eventDataSets.size; i++){
            DataSet curentDataSet = eventDataSets.ptr[i];
            DSetCreatPropList* eventsPlist =  Utils::createDecompressedSetCreatPropList(12);
            DataSet* newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetNames[i], eventDataType, *eventsDataSpaces[i], *eventsPlist));
            newEventsDataset->write(decompressedEventsBuffers[i].ptr, eventDataType);
        }
    }
    // End events

    for (int i = 0; i < signalDataSets.size; i++){
        DataSet curentDataSet = signalDataSets.ptr[i];
        DSetCreatPropList* readsPList = Utils::createDecompressedSetCreatPropList(decompressedSignalBuffers[i].size);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], decompressedSignalDataType, *signalDataSpaces[i], *readsPList));
        newSignalsDataset->write(decompressedSignalBuffers[i].ptr, decompressedSignalDataType, *signalDataSpaces[i], *signalDataSpaces[i]);
    }

}

void removeLogs(H5File* file,string newFileName) {
    Utils::unlinkLogs(file,"/");
    h5repack::repack(file, newFileName, "9");
}

void readTreeFile() {
    ifstream inFile;
    string line;
    int pos;
    string limit = ": ";

    inFile.open("../Files/ganamo.txt");
    if(!inFile){
        cout<<"Error al abrir el archivo";
        exit(1);
    }
    while(getline(inFile,line)){
        string sPos = line.substr(0,line.find(limit));
        string sVal = line.substr(line.find(limit) + 2);
        pos = stoi(sPos);
        treeC.insert(std::pair<int,string>(pos,sVal));
    }
}


void Compresser::CompressFile(H5File* file, int compressionLevel){

    cout<< "Compression Level: " << compressionLevel << endl;
    string compressedFileName = file->getFileName();
    Utils::replaceString(compressedFileName, "_copy.fast5", "_repacked.fast5");
    if(compressionLevel > 0){
        globalAttributes.insert(pair<string,int>("compLevel",compressionLevel));
        readTreeFile();
    }


    if(compressionLevel == 0){
        printChunks(file);
        //generateHuffmanFromExample(file);
    } else if(compressionLevel == 1){
        h5repack::repack(file, compressedFileName, "9");
    } else if(compressionLevel == 2 || compressionLevel == 3){
        compressEventsAndReads(file,compressedFileName,compressionLevel);
    } else if(compressionLevel == 4){
        removeLogs(file,compressedFileName);
    } else if(compressionLevel == 5){
        getOnlyReads(file,compressedFileName);
    } else if(compressionLevel == 6){
        generateHuffmanFromExample(file);
    }

    saveAtributes(compressedFileName);
}

void Compresser::DeCompressFile(H5File* file, int compressionLevel){

    cout<< "Decompression Level: " << compressionLevel << endl;
    string fileName = file->getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, "_copy.fast5", "_deCompressed.fast5");

    string attrName = "compLevel";;
    Group root = file->openGroup("/");
    H5Adelete(root.getId(),attrName.c_str());
    if(compressionLevel == 1){
        h5repack::repack(file, deCompressedFileName, "3");
    } else if(compressionLevel == 2 || compressionLevel == 3){
        deCompressEventsAndReads(file,deCompressedFileName,compressionLevel);
    }
}






