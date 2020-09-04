#include "../headers/inputOutput.hpp"
#include "../headers/compresser.hpp"
#include "../headers/utils.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <chrono>

using namespace std;
using namespace inputOutput;
using namespace compresser;
using namespace utils;
using namespace H5;
using namespace boost::filesystem;

int main (int argc, char* argv[])
{
    //inicia timer de compresion
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    Arguments* args = InputOutput::ProcessArguments(argc, argv);
    if (!args->isOk){
        cout << args->errorMessage;
        exit(1) ;
    }

    Compresser* comp = new Compresser();
    if (args->compress){
        comp->CompressFile(args->file, args->compressionLevel);
    }
    else {
        comp->DeCompressFile(args->file, args->compressionLevel);
    }

    //se elimina el archivo original
    remove(args->fileName);

    //se termina el timer
    chrono::steady_clock::time_point end = chrono::steady_clock::now();

    cout << "Time to compress: " << chrono::duration_cast<chrono::seconds>(end - begin).count() << " seconds";
    exit(0);
}

