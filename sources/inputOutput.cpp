
#include "../headers/inputOutput.hpp"
#include <iostream>
#include <string.h>
#include <sys/stat.h>
using namespace std;
using namespace inputOutput;
using namespace H5;

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

    if (strncmp(compressionLevel, "1", 1) != 0 && strncmp(compressionLevel, "0", 1) != 0){
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
        H5File file(argv[1], H5F_ACC_SWMR_READ);
        arg->file = file;
        arg->isFolder = false;
    }
    else {
        arg->isFolder = true;
    }
    return arg;
}