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
#include <stdio.h>


using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
using namespace h5repack;
using namespace huffman;

map<string,int> globalAttributes;
string treeC[402];


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
            int attValue = it->second;
            int value[1] = { attValue };
            Attribute at = root.createAttribute( attName, PredType::NATIVE_INT,ds);
            at.write( PredType::NATIVE_INT, value);
            it++;
        }
    }
}

MinHeapNode* getHuffmanTreeFromFile() {
    ifstream inFile;
    string line;
    map<string,int> huffmanMap;
    vector<string> huffmanCodes;
    int pos;
    string limit = ": ";

    inFile.open("../Files/ganamo.txt");
    if(!inFile){
        cout<<"Error al abrir el archivo";
        exit(1);
    }
    while(getline(inFile,line)){
        string sPos = line.substr(0,line.find(limit));
        string sVal = line.substr(line.find(limit));
        pos = stoi(sPos);
        huffmanMap[sVal] = pos;
        huffmanCodes.push_back(sVal);
    }
    MinHeapNode* tree = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    Huffman::generateNewTree(huffmanMap, huffmanCodes, tree, "");
    return tree;
}

h5Array<int> mapSignalBufferD(h5Array<int16_t> pChar){
    MinHeapNode* huffmanTree = getHuffmanTreeFromFile();
    string bitstring;
    for(int i = 0;i<pChar.size;i++){
        string aux = bitset<16>(pChar.ptr[i]).to_string();
        bitstring.append(aux);
    }
    int i = 0;
    vector<int> vec;
    while(i<bitstring.size()){
        MinHeapNode* aux_tree = huffmanTree;
        string aux_string = "";
        bool found = false;
        while(!found && i<bitstring.size()){
            char c = bitstring.at(i);
            aux_string.append(c + "");
            if(c = '1'){
                aux_tree = aux_tree->right;
            }else{
                aux_tree = aux_tree->left;
            }
            if(aux_tree->number != 666){
                found = true;
                int leaf = aux_tree->number;
                if(leaf = 201){
                    leaf = Utils::stringToInt(bitstring.substr(i+1,i+17));
                    i = i+16;
                }
                vec.push_back(leaf);
            }
            i++;
        }
    }
}

void generateHuffmanFromExample(H5File file){

    vector<DataSet> vec2;
    map<int,int>::iterator it2;
    map<int,int> readsMap;

    datasetList* signalDataSets = new datasetList {0,vec2};
    Utils::listDatasets("Signal",file,"/", signalDataSets);
    string* signalDatasetNames = new string[signalDataSets->size];
    DataSpace** signalDataSpaces = new DataSpace*[signalDataSets->size];

    int i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        signalDatasetNames[i] = (*it).getObjName();
        signalDataSpaces[i] = new DataSpace((*it).getSpace());

        DataSpace* signalDataSpace = new DataSpace((&*it)->getSpace());
        hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
        signalDataSpace->getSimpleExtentDims(signalDims);
        int signalsCount = (int)(signalDims[0]);
        uint16_t * signalsBuffer = new uint16_t[signalsCount];
        (&*it)->read(signalsBuffer,Utils::getDecompressedSignalDataType(),*signalDataSpace,*signalDataSpace);

        int firstSignal = signalsBuffer[0];
        for(int j = 1; j< signalsCount; j++){
            int diff = signalsBuffer[j] - firstSignal;
            if (diff <= 200 && diff >= -200){
                if (readsMap.find(diff) == readsMap.end()){
                    readsMap[diff] = 1;
                }
                else {
                    readsMap[diff] = readsMap[diff] + 1;
                }
            }
            else {
                if (readsMap.find(201) == readsMap.end()){
                    readsMap[201] = 1;
                }
                else {
                    readsMap[201] = readsMap[201] + 2;
                }
            }

            firstSignal = signalsBuffer[j];
        }

        i++;
    }

    Huffman::generateTree(readsMap);
    ofstream myfile;
    myfile.open ("cuco.txt");

    for(it2 = readsMap.begin(); it2 != readsMap.end(); ++it2) {
        myfile<< it2->first << "," << it2->second << endl;
    }
    myfile.close();
}

h5Array<compressedEventData> getCompressedEventsBuffer(H5File file, DataSet *eventsDataset) {

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

h5Array<eventData> getDecompressedEventsBuffer(H5File file, DataSet *compressedEventsDataset) {

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
    Group root = file.openGroup("/");
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

h5Array<int16_t> getCompressedSignalBuffer(H5File file, DataSet *signalDataset) {

    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
    signalDataSpace->getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    uint16_t* signalsBuffer = new uint16_t[signalsCount];

    int16_t* newSignalsBuffer = new int16_t[signalsCount];
    signalDataset->read(signalsBuffer,Utils::getDecompressedSignalDataType(),*signalDataSpace,*signalDataSpace);

    globalAttributes.insert(pair<string,int>(signalDataset->getObjName() + "_firstRead",signalsBuffer[0]));

    newSignalsBuffer[0] = 0;
    for(int i = 1; i< signalsCount; i++){
        newSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }
    h5Array<int16_t> ret = h5Array<int16_t>(newSignalsBuffer,signalsCount);
    return ret;
}

h5Array<uint16_t> getDecompressedSignalBuffer(H5File file, DataSet *signalDataset,int compressionLevel) {
    Group root = file.openGroup("/");
    string firstReadAttributeName = signalDataset->getObjName() + "_firstRead";
    if(root.attrExists(firstReadAttributeName)) {
        int firstRead;
        Attribute at = root.openAttribute(firstReadAttributeName);
        DataType dt = at.getDataType();
        at.read(dt,&firstRead);

        H5Adelete(root.getId(),firstReadAttributeName.c_str());

        DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
        hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
        signalDataSpace->getSimpleExtentDims(signalDims);
        int signalsCount = (int)(signalDims[0]);
        int realCount;
        int* signalsBuffer;


        switch(compressionLevel){
            case 2: {
                signalsBuffer = new int[signalsCount];
                signalDataset->read(signalsBuffer,PredType::NATIVE_INT,*signalDataSpace,*signalDataSpace);
                realCount = signalsCount;
                break;
            }
            case 3: {
                int16_t* huffmanBuffer = new int16_t[signalsCount];
                signalDataset->read(huffmanBuffer,Utils::getHuffmanSignalDataType(),*signalDataSpace,*signalDataSpace);
                h5Array<int> cuco = mapSignalBufferD(h5Array<int16_t>(huffmanBuffer,signalsCount));
                signalsBuffer = cuco.ptr;
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
    }else
        exit(1);
}

h5Array<int16_t> mapSignalBufferC(h5Array<int16_t> pInt) {
    string bitstring;
    string aux;

    for (int j = 0; j < pInt.size; j++){
        if(abs(pInt.ptr[j])<201) {
            aux = treeC[pInt.ptr[j] + 200];
        }else{
            aux = treeC[402] + bitset<16>(pInt.ptr[j]).to_string();
        }
        bitstring.append(aux);
    }

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

    h5Array<int16_t> ret = h5Array<int16_t>(intArray,count);
    return ret;
}



void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

datasetList* getDataSetList(H5File file,string name){
    vector<DataSet> vec;
    datasetList* ret = new datasetList{0, vec};
    Utils::listDatasets(name,file,"/", ret);
    return ret;
}

void compressEventsAndReads(H5File file,string newFileName,int compLvl){

    datasetList* eventDataSets = getDataSetList(file,"Events");
    datasetList* signalDataSets = getDataSetList(file,"Signal");


    h5Array<compressedEventData>* compressedEventsBuffers = new h5Array<compressedEventData>[eventDataSets->size];
    string* eventsDatasetNames = new string[eventDataSets->size];
    DataSpace** eventsDataSpaces = new DataSpace*[eventDataSets->size];


    h5Array<int16_t>* compressedSignalBuffers = new h5Array<int16_t>[signalDataSets->size];
    string* signalDatasetNames = new string[signalDataSets->size];
    DataSpace** signalDataSpaces = new DataSpace*[signalDataSets->size];

    int i = 0;
    for (vector<DataSet>::iterator it = eventDataSets->ds.begin(); it != eventDataSets->ds.end(); ++it){
        eventsDatasetNames[i] = (*it).getObjName();
        eventsDataSpaces[i] = new DataSpace((*it).getSpace());
        compressedEventsBuffers[i] = getCompressedEventsBuffer(file, &*it);
        unlink(file, (*it).getObjName().c_str());
        i++;
    }

    i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        signalDatasetNames[i] = (*it).getObjName();
        signalDataSpaces[i] = new DataSpace((*it).getSpace());
        compressedSignalBuffers[i] = getCompressedSignalBuffer(file, &*it);
        unlink(file, (*it).getObjName().c_str());
        i++;
    }

    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();;
    i = 0;
    for (vector<DataSet>::iterator it = eventDataSets->ds.begin(); it != eventDataSets->ds.end(); ++it){
        DSetCreatPropList* eventsPlist =  Utils::createCompressedSetCreatPropList(&*it);
        DataSet* newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetNames[i], compressedEventDataType, *eventsDataSpaces[i], *eventsPlist));
        newEventsDataset->write(compressedEventsBuffers[i].ptr, compressedEventDataType);
        i++;
    }

    switch (compLvl) {
        case 2:{
            PredType compressedSignalDataType = Utils::getCompressedSignalDataType();
            i = 0;
            for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
                DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(&*it);
                DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, *signalDataSpaces[i], *readsPList));
                newSignalsDataset->write(compressedSignalBuffers[i].ptr, compressedSignalDataType, *signalDataSpaces[i], *signalDataSpaces[i]);
                i++;
            }
            break;
        }

        case 3:{
            PredType compressedSignalDataType = Utils::getHuffmanSignalDataType();
            i = 0;
            for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
                h5Array<int16_t> huffmanSignalBuffer = mapSignalBufferC(compressedSignalBuffers[i]);
                DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(&*it);


                hsize_t chunk_dims[1] = { (hsize_t)huffmanSignalBuffer.size };
                DSetCreatPropList* creatPropList = new DSetCreatPropList;
                creatPropList->setSzip(H5_SZIP_NN_OPTION_MASK,32);
                creatPropList->setChunk(1, chunk_dims);

                hsize_t current_dims[1] = { (hsize_t)huffmanSignalBuffer.size };
                DataSpace * cuco = new DataSpace(1, current_dims);
                DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, *cuco, *creatPropList));
                newSignalsDataset->write(huffmanSignalBuffer.ptr, compressedSignalDataType, *cuco, *cuco);
                i++;
            }
            break;
        }
        default: exit(1);
    }

}

void getOnlyReads(H5File file,string newFileName){
    DataSet* signalsDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
    string readsDatasetName = signalsDataset->getObjName();
    DataSpace* signalsDataSpace = new DataSpace(signalsDataset->getSpace());
    DataType signalDataType = signalsDataset->getDataType();
    hsize_t signalDims[signalsDataSpace->getSimpleExtentNdims()];
    signalsDataSpace->getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(signalsDataset);

    int16_t* newSignalsBuffer = new int16_t[signalsCount];
    signalsDataset->read(newSignalsBuffer,Utils::getDecompressedSignalDataType(),*signalsDataSpace,*signalsDataSpace);

    H5File newFile(newFileName, H5F_ACC_TRUNC);

    DataSet * newSignalsDataset = new DataSet(newFile.createDataSet("/Signal", signalDataType, *signalsDataSpace, *readsPList));
    newSignalsDataset->write(newSignalsBuffer, signalDataType, *signalsDataSpace, *signalsDataSpace);
}

void deCompressEventsAndReads(H5File file,string newFileName,int compressionLevel){
    vector<DataSet> eVec;
    vector<DataSet> sVec;

    datasetList* eventDataSets = new datasetList{0, eVec};
    datasetList* signalDataSets = new datasetList{0, sVec};

    Utils::listDatasets("Events",file,"/", eventDataSets);
    Utils::listDatasets("Signal",file,"/", signalDataSets);

    h5Array<eventData>* decompressedEventsBuffers = new h5Array<eventData>[eventDataSets->size];
    string* eventsDatasetNames = new string[eventDataSets->size];
    DataSpace** eventsDataSpaces = new DataSpace*[eventDataSets->size];


    h5Array<uint16_t>* decompressedSignalBuffers = new h5Array<uint16_t>[signalDataSets->size];
    string* signalDatasetNames = new string[signalDataSets->size];
    DataSpace** signalDataSpaces = new DataSpace*[signalDataSets->size];

    int i = 0;
    for (vector<DataSet>::iterator it = eventDataSets->ds.begin(); it != eventDataSets->ds.end(); ++it){
        decompressedEventsBuffers[i] = getDecompressedEventsBuffer(file, &*it);
        eventsDatasetNames[i] = (*it).getObjName();
        eventsDataSpaces[i] = new DataSpace((*it).getSpace());
        unlink(file, (*it).getObjName().c_str());
        i++;
    }

    i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        signalDatasetNames[i] = (*it).getObjName();
        signalDataSpaces[i] = new DataSpace((*it).getSpace());
        decompressedSignalBuffers[i] = getDecompressedSignalBuffer(file, &*it,compressionLevel);
        unlink(file, (*it).getObjName().c_str());
        i++;
    }

    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType eventDataType = Utils::getEventDataType();
    PredType decompressedSignalDataType = Utils::getDecompressedSignalDataType();

    i = 0;
    for (vector<DataSet>::iterator it = eventDataSets->ds.begin(); it != eventDataSets->ds.end(); ++it){
        DSetCreatPropList* eventsPlist =  Utils::createDecompressedSetCreatPropList(&*it);
        DataSet* newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetNames[i], eventDataType, *eventsDataSpaces[i], *eventsPlist));
        newEventsDataset->write(decompressedEventsBuffers[i].ptr, eventDataType);
        i++;
    }

    i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        DSetCreatPropList* readsPList = Utils::createDecompressedSetCreatPropList(&*it);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], decompressedSignalDataType, *signalDataSpaces[i], *readsPList));
        newSignalsDataset->write(decompressedSignalBuffers[i].ptr, decompressedSignalDataType, *signalDataSpaces[i], *signalDataSpaces[i]);
        i++;
    }

}

void removeLogs(H5File file,string newFileName) {
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
        treeC[pos + 200] = sVal;
    }
}


void Compresser::CompressFile(H5File file, int compressionLevel){

    cout<< "Compression Level: " << compressionLevel << endl;
    string compressedFileName = file.getFileName();
    Utils::replaceString(compressedFileName, "_copy.fast5", "_repacked.fast5");
    if(compressionLevel > 0){
        globalAttributes.insert(pair<string,int>("compLevel",compressionLevel));
        readTreeFile();
    }


    if(compressionLevel == 0){
        generateHuffmanFromExample(file);
    } else if(compressionLevel == 1){
        h5repack::repack(file, compressedFileName, "9");
    } else if(compressionLevel == 2 || compressionLevel == 3){
        compressEventsAndReads(file,compressedFileName,compressionLevel);
    } else if(compressionLevel == 4){
        removeLogs(file,compressedFileName);
    } else if(compressionLevel == 5){
        getOnlyReads(file,compressedFileName);
    }

    saveAtributes(compressedFileName);
}

void Compresser::DeCompressFile(H5File file, int compressionLevel){

    cout<< "Decompression Level: " << compressionLevel << endl;
    string fileName = file.getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, "_copy.fast5", "_deCompressed.fast5");

    string attrName = "compLevel";;
    Group root = file.openGroup("/");
    H5Adelete(root.getId(),attrName.c_str());
    if(compressionLevel == 1){
        h5repack::repack(file, deCompressedFileName, "3");
    } else if(compressionLevel == 2 || compressionLevel == 3){
        deCompressEventsAndReads(file,deCompressedFileName,compressionLevel);
    }



}






