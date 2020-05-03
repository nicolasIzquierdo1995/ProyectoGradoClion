#include "../headers/utils.hpp"
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <boost/lambda/bind.hpp>
#include <iostream>
#include <cmath>
using namespace std;
using namespace H5;
using namespace boost::filesystem;
using namespace boost::lambda;
using namespace utils;

DataSet* Utils::GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName)
{
    try {
        Group group = file.openGroup(path);
        hsize_t objCount =  group.getNumObjs() ;
        string parentName;
        for (int i = 0; i < objCount; i++){
            string objectName = group.getObjnameByIdx(i);
            if (objectName.find(dataSetGrandParentName) == 0){
                parentName = objectName;
                break;
            }
        }
        DataSet* dataset = new DataSet(file.openDataSet(path + "/" + parentName + "/" + dataSetName));
        return dataset;
    }
    catch(Exception ex) {
        return NULL;
    }
}

int Utils::GetFilesCount(string filePath){
    path the_path(filePath);
    return std::count_if(
            directory_iterator(the_path),
            directory_iterator(),
            static_cast<bool (*)(const path &)>(is_regular_file));
}

string* Utils::GetFileArray(string filePath, int fileCount){
    string* fileArray = new string[fileCount];

    int i = 0;
    typedef vector <path> vec;             // store paths,
    vec v;                                // so we can sort them later

    copy(directory_iterator(filePath), directory_iterator(), back_inserter(v));

    sort(v.begin(), v.end());             // sort, since directory iteration
    // is not ordered on some file systems

    for (vec::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it) {
        fileArray[i] = it->string();
        i++;
    }
    return fileArray;
}

bool Utils::IsInt(DataSet ds){
    H5T_class_t type_class = ds.getTypeClass();
    return type_class == H5T_INTEGER;
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

PredType Utils::getCompressedSignalDataType(){
    return PredType::NATIVE_INT16;
}

PredType Utils::getDecompressedSignalDataType(){
    return PredType::NATIVE_UINT16;
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

DSetCreatPropList *Utils::createDecompressedSetCreatPropList(DataSet* dSet) {
    hsize_t chunk_dims[1];
    dSet->getCreatePlist().getChunk(1, chunk_dims);
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(3);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

PredType Utils::getIntType(int* buffer, int count){
    int max = 0;
    int min = 0;
    for (int i = 0; i < count; i++){
        if (buffer[i] > max){
            max = buffer[i];
        }
        if (buffer[i] < min){
            min = buffer[i];
        }
    }

    if (min >= 0){
        if (max < 256){
            return PredType::STD_U8LE;
        }
        else if (max < 65536){
            return PredType::STD_U16LE;
        }
        else if (max >= 65536){
            return PredType::STD_U32LE;
        }
    }
    else{
        if (max < 128 -1 && abs(min) < 128){
            return PredType::STD_I8LE;
        }
        else if (max < 32767 - 1 && abs(min) < 32767){
            return PredType::STD_I16LE;
        }
        else if (max >= 32767 - 1 && abs(min) >= 32767){
            return PredType::STD_I32LE;
        }
    }
}

StdvAndMean Utils::getStdvAndMean(int* buffer, int start, int length)
{
    float sum = 0.0, mean, standardDeviation = 0.0;

    int i;

    for(i = start; i < start + length; ++i)
    {
        sum += buffer[i];
    }

    mean = sum/length;

    for(i = start; i < start + length; ++i)
        standardDeviation += pow(buffer[i] - mean, 2);

    return {sqrt(standardDeviation / length), mean };
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

void Utils::unlinkLogs(H5File file,string path) {
    Group group = file.openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);
        if (objectName.find("Log") == 0){
            file.unlink(path + objectName);
            break;
        }else if(group.getObjTypeByIdx(i) == H5G_GROUP){
            unlinkLogs(file,path + objectName + "/");
        }
    }
}


