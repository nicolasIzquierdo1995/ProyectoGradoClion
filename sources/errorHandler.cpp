#include "../headers/errorHandler.hpp"
#include "../headers/inputOutput.hpp"

using namespace errorHandler;
using namespace inputOutput;

void ErrorHandler::handleError(int errorCode){
    switch(errorCode) {
        case(1):
            InputOutput::PrintOutput("Error en argumentos");
            break;
        case(2):
            InputOutput::PrintOutput("Error al abrir el archivo\"");
            break;
        case(3):
            InputOutput::PrintOutput("Error en nivel de compresion");
            break;
        case(4):
            InputOutput::PrintOutput("Error al comprimir");
            break;
        case(5):
            InputOutput::PrintOutput("Error al descomprimir");
            break;
    }
    exit(1);
}