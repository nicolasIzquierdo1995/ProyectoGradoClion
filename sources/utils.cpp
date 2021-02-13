#include "../headers/utils.hpp"
#include <sys/stat.h>
#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>

using namespace std;
using namespace H5;
using namespace utils;

//devuelve el tipo de dato de los eventos
CompType Utils::getEventDataType() {
    CompType eventDataType(sizeof(eventData));
    eventDataType.insertMember("start", HOFFSET(eventData,start), PredType::NATIVE_INT);
    eventDataType.insertMember("length", HOFFSET(eventData,length), PredType::NATIVE_INT);
    eventDataType.insertMember("mean", HOFFSET(eventData,mean), PredType::NATIVE_FLOAT);
    eventDataType.insertMember("stdv", HOFFSET(eventData,stdv), PredType::NATIVE_FLOAT);
    return eventDataType;
}

//devuelve el tipo de dato de los eventos comprimidos
CompType Utils::getCompressedEventDataType() {
    CompType compressedEventDataType(sizeof(compressedEventData));
    compressedEventDataType.insertMember("skip", HOFFSET(compressedEventData,skip), PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("length", HOFFSET(compressedEventData,length) , PredType::NATIVE_INT8);
    compressedEventDataType.insertMember("mean", HOFFSET(compressedEventData,mean), PredType::NATIVE_FLOAT);
    compressedEventDataType.insertMember("stdv", HOFFSET(compressedEventData,stdv), PredType::NATIVE_FLOAT);
    return compressedEventDataType;
}

//devuelve el tipo de dato de los signal
PredType Utils::getSignalDataType(){
    return PredType::NATIVE_UINT16;
}

//devuelve el tipo de dato de los signal
PredType Utils::getCompressedSignalDataType(){
    return PredType::NATIVE_INT16;
}

bool Utils::replaceString(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

//crea un propertylist con compresion szip con un chunk de tamano fijo
DSetCreatPropList* Utils::createCompressedSetCreatPropList(int size) {
    hsize_t chunk_dims[1] = { (hsize_t)size };
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setSzip(H5_SZIP_NN_OPTION_MASK, 32);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

//crea un propertylist con compresion gzip 3 con un chunk de tamano fijo
DSetCreatPropList *Utils::createDecompressedSetCreatPropList(int size) {
    hsize_t chunk_dims[1] = { (hsize_t)size };
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setDeflate(3);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

//crea un propertylist con compresion szip con un chunk del mismo tamano que el del dataset de entrada
DSetCreatPropList* Utils::createCompressedSetCreatPropList(DataSet* dSet) {
    hsize_t chunk_dims[1];
    dSet->getCreatePlist().getChunk(1, chunk_dims);
    DSetCreatPropList* creatPropList = new DSetCreatPropList;
    creatPropList->setSzip(H5_SZIP_NN_OPTION_MASK, 32);
    creatPropList->setChunk(1, chunk_dims);
    return creatPropList;
}

void Utils::copyFile(string originalName, string copyName){
        FILE *src1;
        FILE *final;

        src1 = fopen(originalName.c_str(),"rb");
        final = fopen(copyName.c_str(),"wb");

        int c;
        while((c=fgetc(src1))!=EOF)
        {
            fputc(c,final);
        }

        fclose(src1);
        fclose(final);

        struct stat st;
        stat(originalName.c_str(), &st);
        chmod(copyName.c_str(), st.st_mode);
}

//deslinkea recursivamente todos los datasets de logs
void Utils::unlinkLogs(H5File* file,string path) {
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs() ;
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);
        if (objectName.find("Log") == 0){
            file->unlink(path + objectName);
        }
        else if (group.getObjTypeByIdx(i) == H5G_GROUP){
            unlinkLogs(file,path + objectName + "/");
        }
    }
}

//devuelve una lista con todos los datasets de reads, tanto para multireads como para el formato viejo
vector<DataSet>* Utils::listDatasets(string name,H5File* file,string path){
    vector<DataSet>* dataSets = new vector<DataSet>();
    Group group = file->openGroup(path);
    hsize_t objCount =  group.getNumObjs();
    for (int i = 0; i < objCount; i++){
        string objectName = group.getObjnameByIdx(i);

        if (objectName.find("read", 0) == 0 ){
            //formato mutli-read
            dataSets->push_back(group.openDataSet(path+objectName + "/Raw/Signal"));
        }else if(objectName.find("Raw",0) == 0){
            //formato viejo
            Group readsGroup = group.openGroup(path + objectName + "/Reads");
            for(int j = 0; j< readsGroup.getNumObjs();j++){
                string groupName = group.getObjnameByIdx(j);
                if(groupName.find("Read_",0) == 0){
                    dataSets->push_back(group.openDataSet(path+objectName + "/Reads/" + groupName + "/Signal"));
                }
            }
        }
    }
    return dataSets;
}


int Utils::stringToInt(string bitString) {
    int ret = 0;
    for (int position = 0; position < 16; position++) {
        if (bitString.at(position) == '1') {
            ret |= 1 << 15 - position;
        }
    }
    return ret;
}


string Utils::removeChar(string s, char c){
    s.erase(std::remove(s.begin(),s.end(),c),s.end());
    return s;
}

int Utils::getDatasetSize(DataSet* dataSet){
    DataSpace dataSpace = dataSet->getSpace();
    hsize_t dims[dataSpace.getSimpleExtentNdims()];
    dataSpace.getSimpleExtentDims(dims);
    return (int)(dims[0]);
}

int Utils::getDatasetSize(DataSpace* dataSpace){
    hsize_t dims[dataSpace->getSimpleExtentNdims()];
    dataSpace->getSimpleExtentDims(dims);
    return (int)(dims[0]);
}

DataSpace Utils::getDataspace(int size, long long unsigned int maxSize){
    hsize_t chunk_dims1[1] = { (hsize_t)size };
    hsize_t chunk_dims2[1] = { (hsize_t)maxSize };
    return  DataSpace(1, chunk_dims1, chunk_dims2);
}

signalAttributes Utils::getSignalAttributes(H5File* file,string dataSetName) {
    string channelName = dataSetName;
    if (replaceString(channelName, "Raw/Signal", "channel_id")) {
        Group group = file->openGroup(channelName);
        if (group.attrExists("digitisation") && group.attrExists("offset") && group.attrExists("range")) {

            double digitisation = 0.0;
            Attribute digitAtt = group.openAttribute("digitisation");
            DataType dt1 = digitAtt.getDataType();
            digitAtt.read(dt1, &digitisation);
            digitAtt.close();

            double offset = 0;
            Attribute offsetAtt = group.openAttribute("offset");
            DataType dt2 = offsetAtt.getDataType();
            offsetAtt.read(dt2, &offset);
            offsetAtt.close();

            double range = 0.0;
            Attribute rangeAtt = group.openAttribute("range");
            DataType dt3 = rangeAtt.getDataType();
            rangeAtt.read(dt3, &range);
            rangeAtt.close();

            return {range, offset, digitisation};
        } else {
            cout << "Error atributos faltantes";
            exit(1);
        }
    } else {
        cout << "Error al reemplazar nombre atributos";
        exit(1);
    }
}
string Utils::getHuffmanTreeString()  {
    return "9: 00000\n"
           "-9: 00001\n"
           "21: 0001000\n"
           "-46: 0001001000\n"
           "101: 0001001001000\n"
           "-147: 0001001001001000\n"
           "187: 0001001001001001\n"
           "161: 000100100100101\n"
           "131: 00010010010011\n"
           "75: 000100100101\n"
           "-59: 00010010011\n"
           "-93: 0001001010000\n"
           "-110: 00010010100010\n"
           "-168: 00010010100011000\n"
           "-193: 000100101000110010\n"
           "-191: 000100101000110011\n"
           "186: 0001001010001101\n"
           "160: 000100101000111\n"
           "100: 0001001010010\n"
           "130: 00010010100110\n"
           "-127: 000100101001110\n"
           "159: 000100101001111\n"
           "56: 00010010101\n"
           "43: 0001001011\n"
           "-35: 000100110\n"
           "74: 000100111000\n"
           "-75: 000100111001\n"
           "99: 0001001110100\n"
           "-92: 0001001110101\n"
           "-146: 0001001110110000\n"
           "185: 0001001110110001\n"
           "-126: 000100111011001\n"
           "129: 00010011101101\n"
           "-109: 00010011101110\n"
           "158: 000100111011110\n"
           "184: 0001001110111110\n"
           "-145: 0001001110111111\n"
           "-45: 0001001111\n"
           "-21: 0001010\n"
           "-27: 00010110\n"
           "-58: 00010111000\n"
           "73: 000101110010\n"
           "98: 0001011100110\n"
           "128: 00010111001110\n"
           "-144: 0001011100111100\n"
           "-190: 000101110011110100\n"
           "-189: 000101110011110101\n"
           "-166: 00010111001111011\n"
           "157: 000101110011111\n"
           "55: 00010111010\n"
           "-74: 000101110110\n"
           "-91: 0001011101110\n"
           "97: 0001011101111\n"
           "33: 000101111\n"
           "15: 000110\n"
           "-15: 000111\n"
           "8: 00100\n"
           "-8: 00101\n"
           "26: 00110000\n"
           "42: 0011000100\n"
           "72: 001100010100\n"
           "-108: 00110001010100\n"
           "127: 00110001010101\n"
           "156: 001100010101100\n"
           "-125: 001100010101101\n"
           "126: 00110001010111\n"
           "-57: 00110001011\n"
           "-44: 0011000110\n"
           "54: 00110001110\n"
           "96: 0011000111100\n"
           "-90: 0011000111101\n"
           "-73: 001100011111\n"
           "20: 0011001\n"
           "-34: 001101000\n"
           "-167: 00110100100000000\n"
           "-165: 00110100100000001\n"
           "-143: 0011010010000001\n"
           "183: 0011010010000010\n"
           "182: 0011010010000011\n"
           "-107: 00110100100001\n"
           "95: 0011010010001\n"
           "71: 001101001001\n"
           "-56: 00110100101\n"
           "155: 001101001100000\n"
           "-124: 001101001100001\n"
           "125: 00110100110001\n"
           "94: 0011010011001\n"
           "-72: 001101001101\n"
           "53: 00110100111\n"
           "-26: 00110101\n"
           "-20: 0011011\n"
           "7: 00111\n"
           "14: 010000\n"
           "32: 010001000\n"
           "41: 0100010010\n"
           "-43: 0100010011\n"
           "25: 01000101\n"
           "70: 010001100000\n"
           "-89: 0100011000010\n"
           "-106: 01000110000110\n"
           "154: 010001100001110\n"
           "-188: 010001100001111000\n"
           "-187: 010001100001111001\n"
           "-163: 01000110000111101\n"
           "-164: 01000110000111110\n"
           "-162: 01000110000111111\n"
           "124: 01000110001000\n"
           "153: 010001100010010\n"
           "181: 0100011000100110\n"
           "-142: 0100011000100111\n"
           "93: 0100011000101\n"
           "123: 01000110001100\n"
           "-123: 010001100011010\n"
           "-141: 0100011000110110\n"
           "180: 0100011000110111\n"
           "-105: 01000110001110\n"
           "152: 010001100011110\n"
           "151: 010001100011111\n"
           "-71: 010001100100\n"
           "69: 010001100101\n"
           "-55: 01000110011\n"
           "-33: 010001101\n"
           "52: 01000111000\n"
           "-88: 0100011100100\n"
           "92: 0100011100101\n"
           "122: 01000111001100\n"
           "-186: 010001110011010000\n"
           "-185: 010001110011010001\n"
           "-161: 01000111001101001\n"
           "200: 01000111001101010\n"
           "-183: 010001110011010110\n"
           "-184: 010001110011010111\n"
           "-122: 010001110011011\n"
           "91: 0100011100111\n"
           "40: 0100011101\n"
           "-42: 0100011110\n"
           "68: 010001111100\n"
           "-70: 010001111101\n"
           "121: 01000111111000\n"
           "-140: 0100011111100100\n"
           "179: 0100011111100101\n"
           "150: 010001111110011\n"
           "120: 01000111111010\n"
           "-104: 01000111111011\n"
           "-87: 0100011111110\n"
           "90: 0100011111111\n"
           "-7: 01001\n"
           "-14: 010100\n"
           "19: 0101010\n"
           "31: 010101100\n"
           "-54: 01010110100\n"
           "51: 01010110101\n"
           "67: 010101101100\n"
           "178: 0101011011010000\n"
           "-139: 0101011011010001\n"
           "-121: 010101101101001\n"
           "-160: 01010110110101000\n"
           "199: 01010110110101001\n"
           "177: 0101011011010101\n"
           "149: 010101101101011\n"
           "119: 01010110110110\n"
           "118: 01010110110111\n"
           "-69: 010101101110\n"
           "-86: 0101011011110\n"
           "89: 0101011011111\n"
           "-25: 01010111\n"
           "6: 01011\n"
           "-6: 01100\n"
           "-19: 0110100\n"
           "-32: 011010100\n"
           "-41: 0110101010\n"
           "39: 0110101011\n"
           "24: 01101011\n"
           "13: 011011\n"
           "5: 01110\n"
           "-5: 01111\n"
           "-13: 100000\n"
           "-53: 10000100000\n"
           "66: 100001000010\n"
           "-103: 10000100001100\n"
           "-159: 10000100001101000\n"
           "198: 10000100001101001\n"
           "176: 1000010000110101\n"
           "148: 100001000011011\n"
           "-138: 1000010000111000\n"
           "-182: 100001000011100100\n"
           "-181: 100001000011100101\n"
           "-158: 10000100001110011\n"
           "-120: 100001000011101\n"
           "117: 10000100001111\n"
           "50: 10000100010\n"
           "-68: 100001000110\n"
           "-666: 100001000111\n"
           "88: 1000010010000\n"
           "-102: 10000100100010\n"
           "147: 100001001000110\n"
           "175: 1000010010001110\n"
           "-137: 1000010010001111\n"
           "-85: 1000010010010\n"
           "87: 1000010010011\n"
           "65: 100001001010\n"
           "-119: 100001001011000\n"
           "146: 100001001011001\n"
           "116: 10000100101101\n"
           "-157: 10000100101110000\n"
           "-156: 10000100101110001\n"
           "174: 1000010010111001\n"
           "145: 100001001011101\n"
           "115: 10000100101111\n"
           "-52: 10000100110\n"
           "-67: 100001001110\n"
           "-101: 10000100111100\n"
           "-118: 100001001111010\n"
           "-180: 100001001111011000\n"
           "-178: 100001001111011001\n"
           "197: 10000100111101101\n"
           "173: 1000010011110111\n"
           "-84: 1000010011111\n"
           "30: 100001010\n"
           "-31: 100001011\n"
           "18: 1000011\n"
           "4: 10001\n"
           "-4: 10010\n"
           "3: 10011\n"
           "-3: 10100\n"
           "-40: 1010100000\n"
           "38: 1010100001\n"
           "49: 10101000100\n"
           "64: 101010001010\n"
           "86: 1010100010110\n"
           "114: 10101000101110\n"
           "144: 101010001011110\n"
           "-136: 1010100010111110\n"
           "196: 10101000101111110\n"
           "-200: 1010100010111111100\n"
           "-199: 1010100010111111101\n"
           "-179: 101010001011111111\n"
           "-66: 101010001100\n"
           "85: 1010100011010\n"
           "-83: 1010100011011\n"
           "-51: 10101000111\n"
           "-24: 10101001\n"
           "-18: 1010101\n"
           "12: 101011\n"
           "2: 10110\n"
           "-2: 10111\n"
           "1: 11000\n"
           "-1: 11001\n"
           "-12: 110100\n"
           "23: 11010100\n"
           "63: 110101010000\n"
           "-100: 11010101000100\n"
           "143: 110101010001010\n"
           "-135: 1101010100010110\n"
           "172: 1101010100010111\n"
           "113: 11010101000110\n"
           "-117: 110101010001110\n"
           "-134: 1101010100011110\n"
           "171: 1101010100011111\n"
           "48: 11010101001\n"
           "-39: 1101010101\n"
           "84: 1101010110000\n"
           "112: 11010101100010\n"
           "142: 110101011000110\n"
           "-155: 11010101100011100\n"
           "-154: 11010101100011101\n"
           "194: 11010101100011110\n"
           "195: 11010101100011111\n"
           "-65: 110101011001\n"
           "-82: 1101010110100\n"
           "-99: 11010101101010\n"
           "111: 11010101101011\n"
           "62: 110101011011\n"
           "37: 1101010111\n"
           "29: 110101100\n"
           "-30: 110101101\n"
           "-50: 11010111000\n"
           "83: 1101011100100\n"
           "141: 110101110010100\n"
           "-116: 110101110010101\n"
           "110: 11010111001011\n"
           "82: 1101011100110\n"
           "170: 1101011100111000\n"
           "-177: 110101110011100100\n"
           "-198: 1101011100111001010\n"
           "-197: 1101011100111001011\n"
           "-152: 11010111001110011\n"
           "140: 110101110011101\n"
           "-98: 11010111001111\n"
           "47: 11010111010\n"
           "-64: 110101110110\n"
           "-81: 1101011101110\n"
           "169: 1101011101111000\n"
           "-153: 11010111011110010\n"
           "-176: 110101110111100110\n"
           "-175: 110101110111100111\n"
           "-115: 110101110111101\n"
           "109: 11010111011111\n"
           "-38: 1101011110\n"
           "-49: 11010111110\n"
           "61: 110101111110\n"
           "81: 1101011111110\n"
           "192: 11010111111110000\n"
           "193: 11010111111110001\n"
           "-133: 1101011111111001\n"
           "139: 110101111111101\n"
           "168: 1101011111111100\n"
           "-132: 1101011111111101\n"
           "138: 110101111111111\n"
           "0: 11011\n"
           "17: 1110000\n"
           "-23: 11100010\n"
           "36: 1110001100\n"
           "-63: 111000110100\n"
           "-80: 1110001101010\n"
           "108: 11100011010110\n"
           "-97: 11100011010111\n"
           "46: 11100011011\n"
           "28: 111000111\n"
           "11: 111001\n"
           "-11: 111010\n"
           "-17: 1110110\n"
           "22: 11101110\n"
           "-29: 111011110\n"
           "-114: 111011111000000\n"
           "-174: 111011111000001000\n"
           "-173: 111011111000001001\n"
           "-151: 11101111100000101\n"
           "166: 1110111110000011\n"
           "107: 11101111100001\n"
           "80: 1110111110001\n"
           "60: 111011111001\n"
           "137: 111011111010000\n"
           "167: 1110111110100010\n"
           "-131: 1110111110100011\n"
           "106: 11101111101001\n"
           "-79: 1110111110101\n"
           "-62: 111011111011\n"
           "-48: 11101111110\n"
           "79: 1110111111100\n"
           "191: 11101111111010000\n"
           "-196: 1110111111101000100\n"
           "-195: 1110111111101000101\n"
           "-172: 111011111110100011\n"
           "165: 1110111111101001\n"
           "136: 111011111110101\n"
           "-96: 11101111111011\n"
           "59: 111011111111\n"
           "10: 111100\n"
           "-37: 1111010000\n"
           "45: 11110100010\n"
           "-113: 111101000110000\n"
           "-149: 11110100011000100\n"
           "-150: 11110100011000101\n"
           "190: 11110100011000110\n"
           "189: 11110100011000111\n"
           "105: 11110100011001\n"
           "78: 1111010001101\n"
           "-78: 1111010001110\n"
           "-130: 1111010001111000\n"
           "164: 1111010001111001\n"
           "135: 111101000111101\n"
           "104: 11110100011111\n"
           "35: 1111010010\n"
           "-61: 111101001100\n"
           "58: 111101001101\n"
           "-47: 11110100111\n"
           "-22: 11110101\n"
           "16: 1111011\n"
           "-10: 111110\n"
           "-95: 11111100000000\n"
           "-112: 111111000000010\n"
           "134: 111111000000011\n"
           "77: 1111110000001\n"
           "103: 11111100000100\n"
           "-94: 11111100000101\n"
           "-77: 1111110000011\n"
           "44: 11111100001\n"
           "-36: 1111110001\n"
           "-28: 111111001\n"
           "27: 111111010\n"
           "163: 1111110110000000\n"
           "-129: 1111110110000001\n"
           "-148: 11111101100000100\n"
           "-170: 111111011000001010\n"
           "-171: 111111011000001011\n"
           "188: 11111101100000110\n"
           "-194: 1111110110000011100\n"
           "-192: 1111110110000011101\n"
           "-169: 111111011000001111\n"
           "102: 11111101100001\n"
           "133: 111111011000100\n"
           "-111: 111111011000101\n"
           "162: 1111110110001100\n"
           "-128: 1111110110001101\n"
           "132: 111111011000111\n"
           "-60: 111111011001\n"
           "57: 111111011010\n"
           "76: 1111110110110\n"
           "-76: 1111110110111\n"
           "34: 1111110111\n"
           "-16: 1111111";
    }