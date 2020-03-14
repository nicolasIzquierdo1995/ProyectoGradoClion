#include "../libs/h5repack/h5_repack_copy.hpp"
#include "../headers/compresser.hpp"
#include "../headers/repack.hpp"
#include <iostream>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
//using namespace h5repack_copy;
using namespace h5repack;

Compresser::Compresser(){

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

compressedEventData *getEventBuffer(H5File file, DataSet *eventsDataset) {

    CompType eventDataType = Utils::getEventDataType();
    CompType compressedEventDataType = Utils::getCompressedEventDataType();

    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    hsize_t eventsDims[eventsDataSpace->getSimpleExtentNdims()];
    eventsDataSpace->getSimpleExtentDims(eventsDims);
    unsigned long eventsCount = (unsigned long)(eventsDims[0]);
    eventData* eventsBuffer = new eventData[eventsCount];

    compressedEventData* newEventsBuffer = new compressedEventData[eventsCount];
    eventsDataset->read(eventsBuffer,eventDataType,*eventsDataSpace,*eventsDataSpace);

    for(int i = 0; i< eventsCount - 1; i++){
        newEventsBuffer[i].skip = eventsBuffer[i+1].start - (eventsBuffer[i].start + eventsBuffer[i].length);
        newEventsBuffer[i].length = eventsBuffer[i].length;
    }

    newEventsBuffer[eventsCount].skip = 0;
    newEventsBuffer[eventsCount].length = eventsBuffer[eventsCount].length;

    return newEventsBuffer;
}

int *getSignalBuffer(H5File file, DataSet *signalDataset) {

    DataSpace* signalDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalDataSpace->getSimpleExtentNdims()];
    signalDataSpace->getSimpleExtentDims(signalDims);
    unsigned long signalsCount = (unsigned long)(signalDims[0]);
    int* signalsBuffer = new int[signalsCount];

    int* newSignalsBuffer = new int[signalsCount - 1];
    signalDataset->read(signalsBuffer,PredType::NATIVE_INT,*signalDataSpace,*signalDataSpace);

    int first = signalsBuffer[0];
    for(int i = 1; i< signalsCount; i++){
        newSignalsBuffer[i] = signalsBuffer[i] - signalsBuffer[i - 1];
    }

    return newSignalsBuffer;
}

void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

void compressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    compressedEventData * buffer = getEventBuffer(file, eventsDataset);
    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());

    string fileName = file.getFileName();
    string newFileName = fileName;
    Utils::replaceString(newFileName, "_copy.fast5", "_repacked.fast5");

    unlink(file, datasetName);
    h5repack::noMain(fileName, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();
    DSetCreatPropList* eventsPlist = Utils::createCompressedSetCreatPropList();

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(buffer, compressedEventDataType);
}

void deCompressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    compressedEventData * buffer = getEventBuffer(file, eventsDataset);

    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    unlink(file, datasetName);
    //repack(file, 9);

    H5File newFile("../Files/repackedFile.fast5", H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();
    DSetCreatPropList* eventsPlist = Utils::createCompressedSetCreatPropList();

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(buffer, compressedEventDataType);
}

void compressReads(H5File file){
    DataSet* signalsDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
    int * buffer = getSignalBuffer(file, signalsDataset);

    string datasetName = signalsDataset->getObjName();
    DataSpace* signalsDataSpace = new DataSpace(signalsDataset->getSpace());

    string fileName = file.getFileName();
    string newFileName = fileName;
    Utils::replaceString(newFileName, "_copy.fast5", "_repacked2.fast5");

    unlink(file, datasetName);
    h5repack::noMain(fileName, newFileName, "9");

    H5File newFile(newFileName, H5F_ACC_RDWR);

    DSetCreatPropList* pList = Utils::createCompressedSetCreatPropList();

    DataSet * newSignalsDataset = new DataSet(newFile.createDataSet(datasetName, PredType::NATIVE_INT, *signalsDataSpace, *pList));
    newSignalsDataset->write(buffer, PredType::NATIVE_INT);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    string fileName = file.getFileName();
    string compressedFileName = fileName;
    Utils::replaceString(compressedFileName, ".fast5", "_compressed.fast5");
    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        h5repack::noMain(fileName, compressedFileName, "9");
    } else if(compressionLevel == 2){
        compressEvents(file);
    } else if(compressionLevel == 3){
        compressReads(file);
    }
}

void Compresser::DeCompressFile(H5File file, int compressionLevel){

    string fileName = file.getFileName();
    string deCompressedFileName = fileName;
    Utils::replaceString(deCompressedFileName, ".fast5", "_deCompressed.fast5");
    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        h5repack::noMain(fileName, deCompressedFileName, "3");
    } else if(compressionLevel == 2){
        deCompressEvents(file);
    }
}


