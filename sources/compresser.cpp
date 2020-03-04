#include "../headers/h5_repack_copy.hpp"
#include "../headers/compresser.hpp"
#include <iostream>
#include <boost/algorithm/string/replace.hpp>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;
using namespace h5repack;

struct eventData {
    int start;
    int length;
    float mean;
    float stdv;
};

struct newEventData {
    int skip;
    int length;
};

Compresser::Compresser(){

}

void stats(H5File file){
    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);
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

newEventData *getEventBuffer(H5File file, DataSet *eventsDataset) {
    string filePathCompressed = file.getFileName();
    boost::replace_all(filePathCompressed, ".fast5", "Compressed.fast5");

    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);

    CompType newEventDataType(sizeof(newEventData));
    newEventDataType.insertMember("skip", HOFFSET(newEventData, skip), PredType::NATIVE_INT);
    newEventDataType.insertMember("length", HOFFSET(newEventData,length), PredType::NATIVE_INT);

    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    hsize_t eventsDims[eventsDataSpace->getSimpleExtentNdims()];
    eventsDataSpace->getSimpleExtentDims(eventsDims);
    unsigned long eventsCount = (unsigned long)(eventsDims[0]);
    eventData* eventsBuffer = new eventData[eventsCount];

    newEventData* newEventsBuffer = new newEventData[eventsCount];
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
    h5repack::copy_objects(file,"../Files/cuco.fast5");
}

void unlink(H5File file, string groupName) {
    file.unlink(groupName);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    if(compressionLevel == 0){
        stats(file);
    } else if(compressionLevel == 1){
        //gzipCompression(file);
        repack(file);
    } else if(compressionLevel == 2){
        //gzipCompression(file);
        compressEvents(file);
    }
}

void Compresser::compressEvents(H5File file){
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");
    newEventData * buffer = getEventBuffer(file, eventsDataset);

    string datasetName = eventsDataset->getObjName();
    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    unlink(file, datasetName);
    repack(file);

    H5File newFile("../Files/cuco.fast5", H5F_ACC_RDWR);


    CompType newEventDataType(sizeof(newEventData));
    newEventDataType.insertMember("skip", HOFFSET(newEventData, skip), PredType::NATIVE_INT);
    newEventDataType.insertMember("length", HOFFSET(newEventData,length), PredType::NATIVE_INT);

    hsize_t chunk_dims[1] = {20};
    DSetCreatPropList* eventsPlist = new DSetCreatPropList;
    eventsPlist->setDeflate(9);
    eventsPlist->setChunk(1, chunk_dims);


    DataSet * newEventsDataset = new DataSet(newFile.createDataSet(datasetName, newEventDataType, *eventsDataSpace, *eventsPlist));
    newEventsDataset->write(buffer, newEventDataType);
}


