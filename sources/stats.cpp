#include "../headers/stats.hpp"
#include "../headers/utils.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace std;
using namespace H5;
using namespace stats;
using namespace utils;

void Stats::getStats(H5File *file, bool digitisado) {
    vector<DataSet>* signalDataSets = Utils::listDatasets("Signal",file,"/");
    string folderName = "../Files/" + file->getFileName() + (digitisado ? "StatsD" : "StatsND");
    char check = mkdir(folderName.c_str(),0777);
    if(!check){
        for (int i = 0; i < signalDataSets->size(); i++){
            DataSet currentDataSet = signalDataSets->at(i);
            signalAttributes attributes = Utils::getSignalAttributes(file,currentDataSet.getObjName());
            DataSpace* signalDataSpace = new DataSpace(currentDataSet.getSpace());
            int signalsCount = Utils::getDatasetSize(signalDataSpace);
            uint16_t* signalsBuffer = new uint16_t[signalsCount];
            currentDataSet.read(signalsBuffer,Utils::getSignalDataType(),*signalDataSpace,*signalDataSpace);


            ofstream myFile;
            myFile.open (folderName + "/stats" + std::to_string(i) + ".csv");
            for (int j = 0; j<  signalsCount; j++) {
                float val = digitisado ? attributes.range/attributes.digitisation * (signalsBuffer[j] + attributes.offset) : (float) signalsBuffer[j];
                myFile<< val << "," << endl;
            }

            myFile.close();
        }
    }else{
        exit(1);
    }
}

void Stats::getStats(DataSet *dataSet) {

}


