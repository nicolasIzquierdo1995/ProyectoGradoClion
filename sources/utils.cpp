
#include "../headers/utils.hpp"

#include <iostream>
#include <string.h>
#include <stdio.h>
using namespace std;
using namespace utils;
using namespace H5;


Utils::Utils(){
}

DataSet* Utils::GetDataset(H5File file, string path, string dataSetGrandParentName, string dataSetName){
    Group group = file.openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    cout << objCount << endl;
    string groupArray [objCount];
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
};