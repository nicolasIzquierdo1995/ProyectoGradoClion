#include "../headers/utils.hpp"
#include <sys/stat.h>
#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>

using namespace std;
using namespace H5;
using namespace utils;

//devuelve el tipo de dato de los eventos
CompType Utils::getEventDataType() {
    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);
    return eventDataType;
}

//devuelve el tipo de dato de los eventos comprimidos
CompType Utils::getCompressedEventDataType() {
    CompType compressedEventDataType(sizeof(compressedEventData));
    compressedEventDataType.insertMember("skip", HOFFSET(compressedEventData,skip), PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("length", HOFFSET(compressedEventData,length) , PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("mean", HOFFSET(compressedEventData,mean), PredType::NATIVE_FLOAT);
    compressedEventDataType.insertMember("stdv", HOFFSET(compressedEventData,stdv), PredType::NATIVE_FLOAT);
    return compressedEventDataType;
}

//devuelve el tipo de dato de los signal
PredType Utils::getSignalDataType(){
    return PredType::NATIVE_UINT16;
}

//devuelve el tipo de dato de los signal
PredType Utils::getCompressedSignalDataType(){
    return PredType::NATIVE_INT16;
}

bool Utils::replaceString(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

//crea un propertylist con compresion szip con un chunk de tamano fijo
DSetCreatPropList* Utils::createCompressedSetCreatPropList(int size) {
    hsize_t chunk_dims[1] = { (hsize_t)size };
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setSzip(H5_SZIP_NN_OPTION_MASK, 32);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

//crea un propertylist con compresion gzip 3 con un chunk de tamano fijo
DSetCreatPropList *Utils::createDecompressedSetCreatPropList(int size) {
    hsize_t chunk_dims[1] = { (hsize_t)size };
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(3);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

//crea un propertylist con compresion szip con un chunk del mismo tamano que el del dataset de entrada
DSetCreatPropList* Utils::createCompressedSetCreatPropList(DataSet* dSet) {
    hsize_t chunk_dims[1];
    dSet->getCreatePlist().getChunk(1, chunk_dims);
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setSzip(H5_SZIP_NN_OPTION_MASK, 32);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

void Utils::copyFile(string originalName, string copyName){
        FILE *src1;
        FILE *final;

        src1 = fopen(originalName.c_str(),"rb");
        final = fopen(copyName.c_str(),"wb");

        int c;
        while((c=fgetc(src1))!=EOF)
        {
            fputc(c,final);
        }

        fclose(src1);
        fclose(final);

        struct stat st;
        stat(originalName.c_str(), &st);
        chmod(copyName.c_str(), st.st_mode);
}

//deslinkea recursivamente todos los datasets de logs
void Utils::unlinkLogs(H5File* file,string path) {
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);
        if (objectName.find("Log") == 0){
            file->unlink(path + objectName);
        }
        else if (group.getObjTypeByIdx(i) == H5G_GROUP){
            unlinkLogs(file,path + objectName + "/");
        }
    }
}

//devuelve una lista con todos los datasets de reads, tanto para multireads como para el formato viejo
vector<DataSet>* Utils::listDatasets(string name,H5File* file,string path){
    vector<DataSet>* dataSets = new vector<DataSet>();
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs();
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);

        if (objectName.find("read", 0) == 0 ){
            //formato mutli-read
            dataSets->push_back(group.openDataSet(path+objectName + "/Raw/Signal"));
        }else if(objectName.find("Raw",0) == 0){
            //formato viejo
            Group readsGroup = group.openGroup(path + objectName + "/Reads");
            for(int j = 0; j< readsGroup.getNumObjs();j++){
                string groupName = group.getObjnameByIdx(j);
                if(groupName.find("Read_",0) == 0){
                    dataSets->push_back(group.openDataSet(path+objectName + "/Reads/" + groupName + "/Signal"));
                }
            }
        }
    }
    return dataSets;
}


int Utils::stringToInt(string bitString) {
    int ret = 0;
    for (int position = 0; position < 16; position++) {
        if (bitString.at(position) == '1') {
            ret |= 1 << 15 - position;
        }
    }
    return ret;
}


string Utils::removeChar(string s, char c){
    s.erase(std::remove(s.begin(),s.end(),c),s.end());
    return s;
}

int Utils::getDatasetSize(DataSet* dataSet){
    DataSpace dataSpace = dataSet->getSpace();
    hsize_t dims[dataSpace.getSimpleExtentNdims()];
    dataSpace.getSimpleExtentDims(dims);
    return (int)(dims[0]);
}

int Utils::getDatasetSize(DataSpace* dataSpace){
    hsize_t dims[dataSpace->getSimpleExtentNdims()];
    dataSpace->getSimpleExtentDims(dims);
    return (int)(dims[0]);
}

DataSpace Utils::getDataspace(int size, long long unsigned int maxSize){
    hsize_t chunk_dims1[1] = { (hsize_t)size };
    hsize_t chunk_dims2[1] = { (hsize_t)maxSize };
    return  DataSpace(1, chunk_dims1, chunk_dims2);
}

