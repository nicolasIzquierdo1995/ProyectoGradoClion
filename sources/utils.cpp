#include "../headers/utils.hpp"
#include <boost/filesystem.hpp>
#include <boost/lambda/bind.hpp>
#include <iostream>
using namespace std;
using namespace H5;
using namespace boost::filesystem;
using namespace boost::lambda;
using namespace utils;

DataSet* Utils::GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName){
    Group group = file.openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    cout << objCount << endl;
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

CompType Utils::getCompressedEventDataType(size_t totalSize,size_t skipSize,size_t lengthSize,PredType skipType,PredType lengthType) {
    CompType compressedEventDataType(totalSize);
    compressedEventDataType.insertMember("skip", sizeof(long) - skipSize, skipType);
    compressedEventDataType.insertMember("length", (2 * sizeof(long)) -lengthSize, lengthType);
    return compressedEventDataType;
}

bool Utils::replaceString(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

DSetCreatPropList* Utils::createCompressedSetCreatPropList() {
    hsize_t chunk_dims[1] = {20};
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(9);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

PredType Utils::getIntType(long* buffer, int count){
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

size_t Utils::getSize(PredType type) {
    if(type == PredType::STD_U8LE || type == PredType::STD_I8LE){
        return sizeof(int8_t);
    }else if(type == PredType::STD_U16LE || type == PredType::STD_I16LE){
        return sizeof(int16_t);
    }else if(type == PredType::STD_U32LE || type == PredType::STD_I32LE){
        return sizeof(int32_t);
    }
}
