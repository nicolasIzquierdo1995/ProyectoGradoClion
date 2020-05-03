#include "../headers/compresser.hpp"
#include "../headers/repack.hpp"
#include "../headers/utils.hpp"
#include <map>
#include <string>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
using namespace h5repack;

map<string,int> globalAttributes;



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

void stats(H5File file){
    CompType eventDataType = Utils::getEventDataType();
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    hsize_t eventsDims[eventsDataSpace->getSimpleExtentNdims()];
    eventsDataSpace->getSimpleExtentDims(eventsDims);
    eventData* eventsBuffer = new eventData[(unsigned long)(eventsDims[0])];
    eventsDataset->read(eventsBuffer,eventDataType,*eventsDataSpace,*eventsDataSpace);
    int skips[(unsigned long)(eventsDims[0])];
    for(int i = 0; i<(unsigned long)(eventsDims[0]);i++){
        skips[i] = eventsBuffer[i+1].start - (eventsBuffer[i].start + eventsBuffer[i].length);
    }
    for(int j = 0; j<(unsigned long)(eventsDims[0]);j++){
        cout<<skips[j]<<",";
    }
        cout<<endl;
}

compressedEventData* getCompressedEventsBuffer(H5File file, DataSet *eventsDataset) {

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

    return eventBuffer;
}

eventData* getDecompressedEventsBuffer(H5File file, DataSet *compressedEventsDataset) {

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

    return decompressedEventsBuffer;
}

int16_t* getCompressedSignalBuffer(H5File file, DataSet *signalDataset) {

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

    return newSignalsBuffer;
}

uint16_t* getDecompressedSignalBuffer(H5File file, DataSet *signalDataset) {
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
        int* signalsBuffer = new int[signalsCount];

        uint16_t* newSignalsBuffer = new uint16_t[signalsCount];
        signalDataset->read(signalsBuffer,PredType::NATIVE_INT,*signalDataSpace,*signalDataSpace);

        newSignalsBuffer[0] = firstRead;
        for(int i = 1; i< signalsCount; i++){
            newSignalsBuffer[i] = newSignalsBuffer[i-1] + signalsBuffer[i];
        }

        return newSignalsBuffer;
    }else
        return  nullptr;
}

void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

void compressEventsAndReads(H5File file,string newFileName){
    vector<DataSet> vec;
    datasetList* eventDataSets = Utils::listDatasets("Events",file,"/", new datasetList {0,vec});
    vector<DataSet> vec2;
    datasetList* signalDataSets = Utils::listDatasets("Signal",file,"/", new datasetList {0,vec2});

    compressedEventData** compressedEventsBuffers = new compressedEventData*[eventDataSets->size];
    string* eventsDatasetNames = new string[eventDataSets->size];
    DataSpace** eventsDataSpaces = new DataSpace*[eventDataSets->size];


    int16_t** compressedSignalBuffers = new int16_t*[signalDataSets->size];
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

    CompType compressedEventDataType = Utils::getCompressedEventDataType();
    PredType compressedSignalDataType = Utils::getCompressedSignalDataType();

    i = 0;
    for (vector<DataSet>::iterator it = eventDataSets->ds.begin(); it != eventDataSets->ds.end(); ++it){
        DSetCreatPropList* eventsPlist =  Utils::createCompressedSetCreatPropList(&*it);
        DataSet* newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetNames[i], compressedEventDataType, *eventsDataSpaces[i], *eventsPlist));
        newEventsDataset->write(compressedSignalBuffers[i], compressedEventDataType);
    }

    i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(&*it);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], compressedSignalDataType, *signalDataSpaces[i], *readsPList));
        newSignalsDataset->write(compressedSignalBuffers[i], compressedSignalDataType, *signalDataSpaces[i], *signalDataSpaces[i]);
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


void deCompressEventsAndReads(H5File file,string newFileName){
    vector<DataSet> vec;
    datasetList* eventDataSets = Utils::listDatasets("Events",file,"/", new datasetList {0,vec});
    vector<DataSet> vec2;
    datasetList* signalDataSets = Utils::listDatasets("Signal",file,"/", new datasetList {0,vec2});

    eventData** decompressedEventsBuffers = new eventData*[eventDataSets->size];
    string* eventsDatasetNames = new string[eventDataSets->size];
    DataSpace** eventsDataSpaces = new DataSpace*[eventDataSets->size];


    uint16_t** decompressedSignalBuffers = new uint16_t*[signalDataSets->size];
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
        decompressedSignalBuffers[i] = getDecompressedSignalBuffer(file, &*it);
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
        newEventsDataset->write(decompressedEventsBuffers[i], eventDataType);
    }

    i = 0;
    for (vector<DataSet>::iterator it = signalDataSets->ds.begin(); it != signalDataSets->ds.end(); ++it){
        DSetCreatPropList* readsPList = Utils::createDecompressedSetCreatPropList(&*it);
        DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(signalDatasetNames[i], decompressedSignalDataType, *signalDataSpaces[i], *readsPList));
        newSignalsDataset->write(decompressedSignalBuffers[i], decompressedSignalDataType, *signalDataSpaces[i], *signalDataSpaces[i]);
    }

}

void removeLogs(H5File file,string newFileName) {
    Utils::unlinkLogs(file,"/");
    h5repack::repack(file, newFileName, "9");
}


void Compresser::CompressFile(H5File file, int compressionLevel){

    cout<< "Compression Level: " << compressionLevel << endl;
    string compressedFileName = file.getFileName();
    Utils::replaceString(compressedFileName, "_copy.fast5", "_repacked.fast5");
    if(compressionLevel > 0)
        globalAttributes.insert(pair<string,int>("compLevel",compressionLevel));

    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        h5repack::repack(file, compressedFileName, "9");
    } else if(compressionLevel == 2){
        compressEventsAndReads(file,compressedFileName);
    } else if(compressionLevel == 3){
        removeLogs(file,compressedFileName);
    } else if(compressionLevel == 4){
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
    } else if(compressionLevel == 2){
        deCompressEventsAndReads(file,deCompressedFileName);
    }



}




