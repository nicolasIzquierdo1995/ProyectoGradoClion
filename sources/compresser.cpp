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
    globalAttributes.insert(pair<string,int>("firstEvent",originalEventsBuffer[0].start));

    for(int i = 0; i< eventsCount - 1; i++){
        eventBuffer[i].skip  = originalEventsBuffer[i + 1].start - (originalEventsBuffer[i].start + originalEventsBuffer[i].length);
        eventBuffer[i].length = originalEventsBuffer[i].length;
    }

    eventBuffer[eventsCount-1].skip = 0;
    eventBuffer[eventsCount-1].length = originalEventsBuffer[eventsCount-1].length;

    return eventBuffer;
}

eventData* getDecompressedEventsBuffer(H5File file, DataSet *compressedEventsDataset) {

    CompType compressedEventDT = Utils::getCompressedEventDataType();

    DataSpace* compressedEventDS = new DataSpace(compressedEventsDataset->getSpace());
    hsize_t compressedEventsD[compressedEventDS->getSimpleExtentNdims()];
    compressedEventDS->getSimpleExtentDims(compressedEventsD);
    unsigned long eventsCount = (unsigned long)(compressedEventsD[0]);

    DataSet* signalDataset = Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
    signalDataSpace->getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    int* signalsBuffer = new int[signalsCount];

    signalDataset->read(signalsBuffer,PredType::NATIVE_INT,*signalDataSpace,*signalDataSpace);

    eventData* decompressedEventsBuffer = new eventData[eventsCount];
    compressedEventData* compressedEventBuffer = new compressedEventData [eventsCount];;

    compressedEventsDataset->read(compressedEventBuffer,compressedEventDT,*compressedEventDS,*compressedEventDS);

    int start;
    int readStart = 0;
    Group root = file.openGroup("/");
    Attribute at = root.openAttribute("firstEvent");
    DataType dt = at.getDataType();
    at.read(dt,&start);


    for(int i = 0; i< eventsCount; i++){
        int length = compressedEventBuffer[i].length;
        decompressedEventsBuffer[i].length  = length;
        decompressedEventsBuffer[i].start = start;
        StdvAndMean stdvAndMean = Utils::getStdvAndMean(signalsBuffer, readStart, length);
        decompressedEventsBuffer[i].mean  = stdvAndMean.mean;
        decompressedEventsBuffer[i].stdv = stdvAndMean.stdv;
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

    globalAttributes.insert(pair<string,int>("firstRead",signalsBuffer[0]));

    newSignalsBuffer[0] = 0;
    for(int i = 1; i< signalsCount; i++){
        newSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }

    return newSignalsBuffer;
}

uint16_t* getDecompressedSignalBuffer(H5File file, DataSet *signalDataset) {
    Group root = file.openGroup("/");
    if(root.attrExists("firstRead")) {
        int firstRead;
        Attribute at = root.openAttribute("firstRead");
        DataType dt = at.getDataType();
        at.read(dt,&firstRead);

        DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
        hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
        signalDataSpace->getSimpleExtentDims(signalDims);
        int signalsCount = (int)(signalDims[0]);
        int* signalsBuffer = new int[signalsCount];

        uint16_t* newSignalsBuffer = new uint16_t[signalsCount];
        signalDataset->read(signalsBuffer,Utils::getCompressedSignalDataType(),*signalDataSpace,*signalDataSpace);

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

void compressEventsAndReads(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    DataSet* signalsDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");

    int16_t* compressedSignalBuffer = getCompressedSignalBuffer(file, signalsDataset);
    compressedEventData* compressedEventsBuffer = getCompressedEventsBuffer(file, eventsDataset);

    string eventsDatasetName = eventsDataset->getObjName();
    string readsDatasetName = signalsDataset->getObjName();

    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    DataSpace* signalsDataSpace = new DataSpace(signalsDataset->getSpace());

    string newFileName = file.getFileName();
    Utils::replaceString(newFileName, "_copy.fast5", "_repacked.fast5");

    unlink(file, eventsDatasetName);
    unlink(file, readsDatasetName);
    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();
    PredType compressedSignalDataType = Utils::getCompressedSignalDataType();
    DSetCreatPropList* eventsPlist =  Utils::createCompressedSetCreatPropList(eventsDataset);
    DSetCreatPropList* readsPList = Utils::createCompressedSetCreatPropList(signalsDataset);

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(eventsDatasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(compressedEventsBuffer, compressedEventDataType);
    DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(readsDatasetName, compressedSignalDataType, *signalsDataSpace, *readsPList));
    newSignalsDataset->write(compressedSignalBuffer, compressedSignalDataType, *signalsDataSpace, *signalsDataSpace);
}

void deCompressEventsAndReads(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    DataSet* signalsDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");

    eventData * decompressedEventBuffer = getDecompressedEventsBuffer(file, eventsDataset);
    uint16_t* decompressedSignalBuffer = getDecompressedSignalBuffer(file,signalsDataset);

    string eventsDatasetName = eventsDataset->getObjName();
    string readsDatasetName = signalsDataset->getObjName();

    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    DataSpace* signalsDataSpace = new DataSpace(signalsDataset->getSpace());

    string newFileName = file.getFileName();
    Utils::replaceString(newFileName, "_copy.fast5", "_decompressed.fast5");

    //unlink(file, eventsDatasetName);
    unlink(file, readsDatasetName);
    h5repack::repack(file, newFileName, "3");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType eventDataType = Utils::getEventDataType();
    PredType decompressedSignalDataType = Utils::getDecompressedSignalDataType();
    DSetCreatPropList* eventsPlist = Utils::createDecompressedSetCreatPropList(eventsDataset);
    DSetCreatPropList* readspList = Utils::createDecompressedSetCreatPropList(signalsDataset);

    //DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, eventDataType, *eventsDataSpace, *eventsPlist));
    //newEventsDataset->write(decompressedEventBuffer, eventDataType);
    DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(readsDatasetName, decompressedSignalDataType, *signalsDataSpace, *readspList));
    newSignalsDataset->write(decompressedSignalBuffer, decompressedSignalDataType);
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
        compressEventsAndReads(file);
    }

    saveAtributes(compressedFileName);
}

void Compresser::DeCompressFile(H5File file, int compressionLevel){

    cout<< "Decompression Level: " << compressionLevel << endl;
    string fileName = file.getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, "_copy.fast5", "_deCompressed.fast5");
    if(compressionLevel == 1){
        h5repack::repack(file, deCompressedFileName, "3");
    } else if(compressionLevel == 2){
        deCompressEventsAndReads(file);
    }
}


