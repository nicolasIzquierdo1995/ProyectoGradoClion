#include "../libs/h5repack/h5_repack_copy.hpp"
#include "../headers/compresser.hpp"
#include "../headers/repack.hpp"
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
}

EventsAndType getEventBuffer(H5File file, DataSet *eventsDataset) {

    CompType originalEventDT = Utils::getEventDataType();

    DataSpace* originalEventDS = new DataSpace(eventsDataset->getSpace());
    hsize_t originalEventsD[originalEventDS->getSimpleExtentNdims()];
    originalEventDS->getSimpleExtentDims(originalEventsD);
    unsigned long eventsCount = (unsigned long)(originalEventsD[0]);
    eventData* originalEventsBuffer = new eventData[eventsCount];

    long* skipAuxBuffer = new long [eventsCount];
    long* lengthAuxBuffer = new long [eventsCount];
    compressedEventData* eventBuffer = new compressedEventData [eventsCount];;

    eventsDataset->read(originalEventsBuffer,originalEventDT,*originalEventDS,*originalEventDS);
    globalAttributes.insert(pair<string,int>("firstEvent",originalEventsBuffer[0].start));



    for(int i = 0; i< eventsCount - 1; i++){
        skipAuxBuffer[i] = originalEventsBuffer[i + 1].start - (originalEventsBuffer[i].start + originalEventsBuffer[i].length);
        lengthAuxBuffer[i] = originalEventsBuffer[i].length;

        eventBuffer[i].skip = skipAuxBuffer[i];
        eventBuffer[i].length = lengthAuxBuffer[i];
    }

    skipAuxBuffer[eventsCount] = 0;
    lengthAuxBuffer[eventsCount] = originalEventsBuffer[eventsCount].length;

    eventBuffer[eventsCount].length = skipAuxBuffer[eventsCount];
    eventBuffer[eventsCount].skip = skipAuxBuffer[eventsCount];

    PredType skipType = Utils::getIntType(skipAuxBuffer, eventsCount);
    PredType lengthType = Utils::getIntType(lengthAuxBuffer, eventsCount);
    size_t skipSize = Utils::getSize(skipType);
    size_t lengthSize = Utils::getSize(lengthType);

    EventsAndType ret {eventBuffer,skipSize + lengthSize,skipSize,skipType,lengthType};

    return ret;
}

ReadsAndType getSignalBuffer(H5File file, DataSet *signalDataset) {

    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
    signalDataSpace->getSimpleExtentDims(signalDims);
    int signalsCount = (int)(signalDims[0]);
    int* signalsBuffer = new int[signalsCount];

    long* newSignalsBuffer = new long[signalsCount - 1];
    signalDataset->read(signalsBuffer,PredType::NATIVE_INT,*signalDataSpace,*signalDataSpace);

    globalAttributes.insert(pair<string,int>("firstRead",signalsBuffer[0]));
    for(int i = 1; i< signalsCount; i++){
        newSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }

    ReadsAndType ret {newSignalsBuffer, Utils::getIntType(newSignalsBuffer,signalsCount)};
    return ret;
}

void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

void compressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    EventsAndType eat = getEventBuffer(file, eventsDataset);
    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());

    string newFileName = file.getFileName();
    Utils::replaceString(newFileName, "_copy.fast5", "_repacked.fast5");

    unlink(file, datasetName);
    h5repack::repack(file, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType(eat.totalSize,eat.offset,eat.skipType,eat.lengthType);
    DSetCreatPropList* eventsPlist = Utils::createCompressedSetCreatPropList();

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(eat.eventBuffer, compressedEventDataType);
}

void deCompressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    compressedEventData * buffer = getEventBuffer(file, eventsDataset).eventBuffer;

    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    unlink(file, datasetName);
    //repack(file, 9);

    H5File newFile("../Files/repackedFile.fast5", H5F_ACC_RDWR);

    CompType compressedEventDataType ;//= Utils::getCompressedEventDataType(PredType::NATIVE_INT,PredType::NATIVE_INT);
    DSetCreatPropList* eventsPlist = Utils::createCompressedSetCreatPropList();

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(buffer, compressedEventDataType);
}

void compressReads(H5File file){
    DataSet* signalsDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
    ReadsAndType rat = getSignalBuffer(file, signalsDataset);

    string datasetName = signalsDataset->getObjName();
    DataSpace* signalsDataSpace = new DataSpace(signalsDataset->getSpace());

    string newFileName = file.getFileName();
    Utils::replaceString(newFileName, "_copy.fast5", "_repacked.fast5");

    unlink(file, datasetName);
    h5repack::repack(file, newFileName, "9");
    H5File newFile(newFileName, H5F_ACC_RDWR);

    DSetCreatPropList* pList = Utils::createCompressedSetCreatPropList();

    DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(datasetName, rat.type, *signalsDataSpace, *pList));
    newSignalsDataset->write(rat.buffer, rat.type);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    string compressedFileName = file.getFileName();
    Utils::replaceString(compressedFileName, "_copy.fast5", "_repacked.fast5");
    if(compressionLevel > 0){
        globalAttributes.insert(pair<string,int>("compLevel",compressionLevel));
    }

    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        h5repack::repack(file, compressedFileName, "9");
    } else if(compressionLevel == 2){
        compressEvents(file);
    } else if(compressionLevel == 3){
        compressReads(file);
    }

    saveAtributes(compressedFileName);
}

void Compresser::DeCompressFile(H5File file, int compressionLevel){

    string fileName = file.getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, "_copy.fast5", "_deCompressed.fast5");
    if(compressionLevel == 1){
        h5repack::repack(file, deCompressedFileName, "3");
    } else if(compressionLevel == 2){
        deCompressEvents(file);
    }
}


