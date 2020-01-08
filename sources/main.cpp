#include "../headers/inputOutput.hpp"
#include "../headers/compresser.hpp"
#include "../headers/threadPool.hpp"
#include <boost/filesystem.hpp>
#include <boost/lambda/bind.hpp>
#include <iostream>
#include <future>
#include <iostream>
using namespace std;
using namespace inputOutput;
using namespace compresser;
using namespace threadPool;
using namespace H5;
using namespace boost::filesystem;
using namespace boost::lambda;

int main (int argc, char* argv[])
{
    Arguments* args = InputOutput::ProcessArguments(argc, argv);
    Compresser* comp = new Compresser();
    ThreadPool* threadPool = new ThreadPool(2);
    if (!args->isFolder){
        comp->CompressFile(args->file, args->compressionLevel);
    }
    else {
        path the_path(argv[1]);

        int fileCount = std::count_if(
                directory_iterator(the_path),
                directory_iterator(),
                static_cast<bool (*)(const path &)>(is_regular_file));

        string fileArray[fileCount];
        future<void> promisesArray[fileCount];
        int i = 0;
        typedef vector <path> vec;             // store paths,
        vec v;                                // so we can sort them later

        copy(directory_iterator(argv[1]), directory_iterator(), back_inserter(v));

        sort(v.begin(), v.end());             // sort, since directory iteration
        // is not ordered on some file systems

        for (vec::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it) {
            fileArray[i] = it->string();
            i++;
        }

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

