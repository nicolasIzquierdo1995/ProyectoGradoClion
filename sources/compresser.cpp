
#include "../headers/compresser.hpp"
#include "../headers/utils.hpp"
#include <iostream>
#include <boost/algorithm/string/replace.hpp>
using namespace std;
using namespace compresser;
using namespace H5;
using namespace utils;


Compresser::Compresser(){

}

void gzipCompression(H5File file){
    string filePathCompressed = file.getFileName();
    string filePathUncompressed = filePathCompressed;
    boost::replace_all(filePathCompressed, ".fast5", "Compressed.fast5");
    boost::replace_all(filePathUncompressed, ".fast5", "Uncompressed.fast5");
    hsize_t chunk_dims[1] = {20};
    DataSet* originalDataset =  Utils::GetDataset(file, "/Raw/Reads", "Read", "Signal");
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

    H5File* compressedFile = new H5File(filePathCompressed,H5F_ACC_TRUNC);
    H5File* unCompressedFile = new H5File(filePathUncompressed,H5F_ACC_TRUNC);
    DataSet* dataSet1 = new DataSet(compressedFile->createDataSet("coquito",dt,*dataSpace,*plist1));
    DataSet* dataSet2 = new DataSet(unCompressedFile->createDataSet("coquito",dt,*dataSpace,*plist2));
    dataSet1->write(buffer,dt);
    dataSet2->write(buffer,dt);
}

void Compresser::CompressFile(H5File file, int compressionLevel){

    if(compressionLevel == 1){
        gzipCompression(file);
    }   

}