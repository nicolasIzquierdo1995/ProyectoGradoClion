#include "../headers/inputOutput.hpp"
#include "../headers/compresser.hpp"
#include "../headers/threadPool.hpp"
#include <iostream>
#include <future>
using namespace std;
using namespace inputOutput;
using namespace compresser;
using namespace threadPool;
using namespace H5;


int main (int argc, char* argv[])
{
    Arguments* args = InputOutput::ProcessArguments(argc, argv);
    Compresser* comp = new Compresser();
    ThreadPool* threadPool = new ThreadPool(2);
    future<void> result = threadPool->enqueue([](Compresser* comp, Arguments* args) { comp->CompressFile(args->file, args->compressionLevel); },
            comp, args);
    result.wait();
    return 0;
}

