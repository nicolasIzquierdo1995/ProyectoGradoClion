
#include "../headers/inputOutput.hpp"
#include <iostream>
#include <string.h>
#include <fstream>
#include <sys/stat.h>
using namespace std;
using namespace inputOutput;
using namespace H5;

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

static bool VerifyArguments(int argc, char *argv[]){
    if (argc != 4){
      return false;
    }
    
    const char* multiThreading = argv[2];
    const char* compressionLevel = argv[3];

    cout << multiThreading << endl;
    cout << compressionLevel << endl;
    if (strncmp(multiThreading, "true", 4) != 0 && strncmp(multiThreading, "false", 4) != 0 ) {
      return false;
    }

    if (strncmp(compressionLevel, "1", 1) != 0 && strncmp(compressionLevel, "0", 1) != 0 && strncmp(compressionLevel, "2", 1) != 0){
     return false;
    }

    return true;
}


static Arguments* CreateErrorArgument(){
    Arguments arg;
    Arguments * parg;
    parg = &arg;
    parg->isOk = false;
    parg->errorMessage = "Error en argumentos";
    return parg;
}

Arguments* InputOutput::ProcessArguments(int argc, char* argv[]){
    if (!VerifyArguments(argc,argv)){
        return CreateErrorArgument();
    }
    Arguments * arg = new Arguments();
    arg->multiThreading = strncmp(argv[2],"false",4);
    arg->compressionLevel = atoi(argv[3]);
    arg->isOk = true;


    struct stat path_stat;
    stat(argv[1], &path_stat);
    bool isDirectory = S_ISDIR(path_stat.st_mode);

    if (!isDirectory){
        string fileName = argv[1];
        replace(fileName, ".fast5", "_copy.fast5");
        ifstream src(argv[1], ios::binary);
        ofstream dst(fileName, ios::binary);
        dst << src.rdbuf();

        H5File file(fileName, H5F_ACC_RDWR);
        arg->file = file;
        arg->fileName = fileName;
        arg->isFolder = false;
    }
    else {
        arg->isFolder = true;
    }
    return arg;
}