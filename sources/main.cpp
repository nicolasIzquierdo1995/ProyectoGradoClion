#include "../headers/inputOutput.hpp"
#include "../headers/compresser.hpp"
#include "../headers/threadPool.hpp"
#include "../headers/utils.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include <future>
using namespace std;
using namespace inputOutput;
using namespace compresser;
using namespace threadPool;
using namespace utils;
using namespace H5;
using namespace boost::filesystem;
using namespace boost::lambda;

int main (int argc, char* argv[])
{
    Arguments* args = InputOutput::ProcessArguments(argc, argv);
    Compresser* comp = new Compresser();
    ThreadPool* threadPool = new ThreadPool(2);
    if (!args->isFolder){
        if (args->compress){
            comp->CompressFile(args->file, args->compressionLevel);
        }
        else {
            comp->DeCompressFile(args->file, args->compressionLevel);
        }
        remove(args->fileName);
    }
    else {
        int fileCount = 1;//Utils::GetFilesCount(argv[1]);
        string* fileArray = NULL;//Utils::GetFileArray(argv[1], fileCount);
        future<void> promisesArray[fileCount];

        for (int j = 0; j < fileCount; j++) {
            H5File file(fileArray[j], H5F_ACC_SWMR_READ);
            args->file = file;
            promisesArray[j] = threadPool->enqueue(
                    [](Compresser *comp, Arguments *args) { comp->CompressFile(args->file, args->compressionLevel); },
                    comp, args);
        }

        for (int j = 0; j < fileCount; j++) {
            promisesArray[j].wait();
        }
    }
    return 0;
}

