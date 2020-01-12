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