
#include "../headers/compresser.hpp"
#include "../headers/utils.hpp"

#include <iostream>
#include <string.h>
#include <stdio.h>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;


Compresser::Compresser(){

}

void gzipCompression(H5File file){
    hsize_t chunk_dims[1] = {20};
    Utils* utils = new Utils();
    DataSet* originalDataset =  utils->GetDataset(file, "/Raw/Reads", "Read", "Signal");
    DataType dt = originalDataset->getDataType();
    DataSpace* dataSpace = new DataSpace(originalDataset->getSpace());
    hsize_t dims[dataSpace->getSimpleExtentNdims()];
    dataSpace->getSimpleExtentDims(dims);
    int buffer[dims[0]];

    DSetCreatPropList* plist1 = new  DSetCreatPropList;
    plist1->setDeflate(0);
    plist1->setChunk(1, chunk_dims);

    DSetCreatPropList* plist2 = new DSetCreatPropList;
    plist2->setDeflate(9);
    plist2->setChunk(1, chunk_dims);

    originalDataset->read(buffer,dt,*dataSpace,*dataSpace);

    H5File* failsito1 = new H5File("noCompression.fast5",H5F_ACC_TRUNC);
    H5File* failsito2 = new H5File("compressed.fast5",H5F_ACC_TRUNC);
    DataSet* dataSet1 = new DataSet(failsito1->createDataSet("coquito",dt,*dataSpace,*plist1));  
    DataSet* dataSet2 = new DataSet(failsito2->createDataSet("coquito",dt,*dataSpace,*plist2));    
    dataSet1->write(buffer,dt);
    dataSet2->write(buffer,dt);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    if(compressionLevel == 1){
        gzipCompression(file);
    }   

}