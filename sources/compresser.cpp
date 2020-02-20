
#include "../headers/compresser.hpp"
#include "../headers/utils.hpp"
#include <iostream>
#include <boost/algorithm/string/replace.hpp>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;

struct eventData {
    int start;
    int length;
    float mean;
    float stdv;
};

Compresser::Compresser(){

}

void gzipCompression(H5File file){
    string filePathCompressed = file.getFileName();
    boost::replace_all(filePathCompressed, ".fast5", "Compressed.fast5");

    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);

    hsize_t chunk_dims[1] = {20};
    DataSet* signalDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
    DataSet* eventsDataset =  Utils::GetDataset(file, "/Analyses/EventDetection_000/Reads", "Read", "Events");

    DataType signalDt = signalDataset->getDataType();
    DataSpace* signalsDataSpace = new DataSpace(signalDataset->getSpace());
    hsize_t signalDims[signalsDataSpace->getSimpleExtentNdims()];
    signalsDataSpace->getSimpleExtentDims(signalDims);
    int signalBuffer[signalDims[0]];

    DataSpace* eventsDataSpace = new DataSpace(eventsDataset->getSpace());
    hsize_t eventsDims[eventsDataSpace->getSimpleExtentNdims()];
    eventsDataSpace->getSimpleExtentDims(eventsDims);
    eventData* eventsBuffer = new eventData[(unsigned long)(eventsDims[0])];

    DSetCreatPropList* signalPlist = new DSetCreatPropList;
    signalPlist->setDeflate(9);
    signalPlist->setChunk(1, chunk_dims);

    DSetCreatPropList* eventsPlist = new DSetCreatPropList;
    eventsPlist->setDeflate(9);
    eventsPlist->setChunk(1, chunk_dims);

    signalDataset->read(signalBuffer,signalDt,*signalsDataSpace,*signalsDataSpace);
    eventsDataset->read(eventsBuffer,eventDataType,*eventsDataSpace,*eventsDataSpace);

    H5File* compressedFile = new H5File(filePathCompressed,H5F_ACC_TRUNC);
    DataSet* dataSet1 = new DataSet(compressedFile->createDataSet("Signal",signalDt,*signalsDataSpace,*signalPlist));
    dataSet1->write(signalBuffer,signalDt);
    DataSet* dataSet2 = new DataSet(compressedFile->createDataSet("Events",eventDataType,*eventsDataSpace,*eventsPlist));
    dataSet2->write(eventsBuffer,eventDataType);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    if(compressionLevel == 1){
        gzipCompression(file);
    }   

}