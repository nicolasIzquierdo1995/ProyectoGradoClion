#include "../headers/utils.hpp"
#include <sys/stat.h>
#include <iostream>
#include <cmath>
using namespace std;
using namespace H5;
using namespace utils;

DataSet* Utils::GetDataset(H5File* file, string path, string dataSetGrandParentName, string dataSetName)
{
    try {
        Group group = file->openGroup(path);
        hsize_t objCount =  group.getNumObjs() ;
        string parentName;
        for (int i = 0; i < objCount; i++){
            string objectName = group.getObjnameByIdx(i);
            if (objectName.find(dataSetGrandParentName) == 0){
                parentName = objectName;
                break;
            }
        }
        DataSet* dataset = new DataSet(file->openDataSet(path + "/" + parentName + "/" + dataSetName));
        return dataset;
    }
    catch(Exception ex) {
        return NULL;
    }
}

CompType Utils::getEventDataType() {
    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);
    return eventDataType;
}

CompType Utils::getCompressedEventDataType() {
    CompType compressedEventDataType(sizeof(compressedEventData));
    compressedEventDataType.insertMember("skip", HOFFSET(compressedEventData,skip), PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("length", HOFFSET(compressedEventData,length) , PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("mean", HOFFSET(compressedEventData,mean), PredType::NATIVE_FLOAT);
    compressedEventDataType.insertMember("stdv", HOFFSET(compressedEventData,stdv), PredType::NATIVE_FLOAT);
    return compressedEventDataType;
}

PredType Utils::getHuffmanSignalDataType(){
    return PredType::NATIVE_INT16;
}

PredType Utils::getCompressedSignalDataType(){
    return PredType::NATIVE_INT16;
}

PredType Utils::getDecompressedSignalDataType(){
    return PredType::NATIVE_INT16;
}

bool Utils::replaceString(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

DSetCreatPropList* Utils::createCompressedSetCreatPropList(DataSet* dSet) {
    hsize_t chunk_dims[1];
    dSet->getCreatePlist().getChunk(1, chunk_dims);
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(9);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

DSetCreatPropList *Utils::createDecompressedSetCreatPropList(int size) {
    hsize_t chunk_dims[1] = { (hsize_t)size };
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(3);
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

void Utils::unlinkLogs(H5File* file,string path) {
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);
        if (objectName.find("Log") == 0){
            file->unlink(path + objectName);
            break;
        }else if(group.getObjTypeByIdx(i) == H5G_GROUP){
            unlinkLogs(file,path + objectName + "/");
        }
    }
}


vector<DataSet>* Utils::listDatasets(string name,H5File* file,string path){
    vector<DataSet>* dataSets = new vector<DataSet>();
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs();
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);
        if (objectName.find("read", 0) == 0 ){
            dataSets->push_back(group.openDataSet(path+objectName + "/Raw/Signal"));
        }else if(objectName.find("Raw",0) == 0){
            dataSets->push_back(group.openDataSet(path+objectName + "/Reads/Read_627/Signal"));
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


