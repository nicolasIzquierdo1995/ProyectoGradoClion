#include "../headers/h5_repack_copy.hpp"
#include "../headers/compresser.hpp"
#include <iostream>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
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

void repack(H5File file) {
    h5repack::copy_objects(file,"../Files/repackedFile.fast5");
}

void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        repack(file);
    } else if(compressionLevel == 2){
        compressEvents(file);
    }
}

void Compresser::compressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    compressedEventData * buffer = getEventBuffer(file, eventsDataset);

    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    unlink(file, datasetName);
    repack(file);

    H5File newFile("../Files/repackedFile.fast5", H5F_ACC_RDWR);

    CompType compressedEventDataType = Utils::getCompressedEventDataType();
    DSetCreatPropList* eventsPlist = Utils::createCompressedSetCreatPropList();

    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, compressedEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(buffer, compressedEventDataType);
}


