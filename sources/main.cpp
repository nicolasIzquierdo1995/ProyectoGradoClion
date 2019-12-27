#include "/home/osboxes/Desktop/myhdfstuff/HDF5-1.10.5-Linux/HDF_Group/HDF5/1.10.5/include/hdf5.h"
#include "../headers/inputOutput.hpp"
#include "../headers/compresser.hpp"
#include <iostream>
using namespace std;
using namespace inputOutput;
using namespace compresser;

int main (int argc, char* argv[])
{
    InputOutput* io = new InputOutput();
    Arguments* args = io->ProcessArguments(argc, argv);
    Compresser* comp = new Compresser();
    comp->CompressFile(args->file, args->compressionLevel);
    return 0;
}

