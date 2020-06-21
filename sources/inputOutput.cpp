#include "../headers/inputOutput.hpp"
#include <iostream>
#include <sys/stat.h>
#include "../headers/utils.hpp"
using namespace std;
using namespace inputOutput;
using namespace H5;
using namespace utils;

static bool VerifyArguments(int argc, char *argv[]){
    if (argc < 2 || argc > 3){
      return false;
    }
    if(argc == 3){
        const char* compressionLevel = argv[2];
        int compInt = std::stoi(compressionLevel,0,10);
        if (compInt < 0 || compInt>4){
            return false;
        }
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
    arg->isOk = true;
    arg->compress = argc == 3;

    struct stat path_stat;
    stat(argv[1], &path_stat);

    string fileName = argv[1];
    Utils::replaceString(fileName, ".fast5", "_copy.fast5");
    Utils::copyFile(argv[1], fileName);

    H5File* file = new H5File(fileName, H5F_ACC_RDWR);
    arg->file = file;
    arg->fileName = fileName;


    if (arg->compress)
        arg->compressionLevel = atoi(argv[2]);
    else {
        Group root = arg->file->openGroup("/");
        if(root.attrExists("compLevel")) {
            int compLvl;
            Attribute at = root.openAttribute("compLevel");
            DataType dt = at.getDataType();
            at.read(dt,&compLvl);
            arg->compressionLevel = compLvl;
        }else
            return  CreateErrorArgument();

    }

    return arg;
}