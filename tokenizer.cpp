///////////////
//
// File name: tokenizer.cpp
// Author: Aaron Leonhard
// Course: COSC4963
// Assignment: 01
//
// Computer Specs: Windows 11, Intel(R) Core(TM) i7-8665U CPU @ 1.90GHz, 16.0 GB RAM, Intel(R) UHD Graphics 620, 477 GB Storage, 64-bit, x64-based processor
// Compiler: G++, C++11
//
// Compile/execution command line(s) g++ -std=c++11 tokenizer.cpp
//
// Program Purpose: Make token map from a text file then be able to construct the original text from the token map.
//
// Inputs: see README.md
//
// Outputs: Json file of vocab and merges, or original text file as dictated by the user.
//
// Algorithm: Byte Pair Encoding (BPE)
//
// Limitations: This program will operate within the bounds of your compute capabilities.
//
// Operational Status: Functional
/////////////

#include<iostream>
#include<fstream>
#include<unordered_map>
#include"nlohmann/json.hpp"
#include<cctype>

using json = nlohmann::json;

void printMap(std::unordered_map<std::string, int>& map){
    std::cout << "current state of map:\n";
    for (auto i : map){
        std::cout << i.first << ": " << i.second << std::endl;
    }
}

void printVector(std::vector<int> &vector){
    std::cout << "current state of vector:\n";
    for (int i = 0; i < vector.size(); i++) {
        std::cout << vector[i] << " ";
    }
}

void printVectorClean(std::vector<std::string> &vector){
    for (int i = 0; i < vector.size(); i++) {
        std::string token = vector[i];
        // markers are usually glued onto a merged token ("e<w>"), so remove them as substrings
        for (const char* marker : {"<w>", "<t>"}) {
            size_t pos;
            while ((pos = token.find(marker)) != std::string::npos) token.erase(pos, 3);
        }
        std::cout << token;
    }
}

void printVectorDirty(std::vector<std::string> &vector){
    for (int i = 0; i < vector.size(); i++) {
        std::cout << vector[i];
    }
}

static bool isPunct(char aChar){
    return aChar == '.' || aChar == '\n' || aChar == '!' || aChar == '?' || aChar == ',' || aChar == ':' || aChar == ';';
}

void mergeCorpus(std::vector<std::string>& corpus, const std::string& first, const std::string& second){
    //we don't care about corpus order, so we can dump anything we want into corpus at the end.
    std::vector<std::string> output;

    for (size_t i = 0; i < corpus.size(); ) {
        if (i + 1 < corpus.size() && corpus[i] == first && corpus[i + 1] == second) {
            // if not end of corpus, and we have found a pair
            output.push_back(first + second);
            i += 2;
        } 
        else {
            // put into output, and move forward
            output.push_back(std::move(corpus[i]));
            i++;
        }
    }
    // dump output into corpus
    corpus = std::move(output);
}

void detokenize(){
    std::unordered_map<int, std::string> vocabSwapped;
    bool useSpecialChars = true;
    std::ifstream jsonFile("vocab.json");
    if (!jsonFile) {
        throw std::runtime_error("could not find vocab.json");
    }

    json j = json::parse(jsonFile);
    int count = 0;

    for (const auto& vocabE : j["vocab"]) {
        std::string token;

        for (const auto& byte : vocabE) {
            token.push_back(static_cast<char>(byte.get<unsigned int>()));
        }
        
        vocabSwapped[count] = token;
        count++;
    }

    std::string inputString;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Output special tokens '0' do not output special tokens '1'\n";
    inputString = std::getchar();

    if(inputString == "0"){
        useSpecialChars = true;
    }
    else{
        useSpecialChars = false;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Input tokens to detokenize: \n";
    std::getline(std::cin, inputString);

    std::vector<std::string> output;
    int tokenNum = 0;
    bool inNumber = false; // true if we're reading nums

    for (uint16_t i = 0; i <= inputString.size(); ++i) {
        char valAtIndex;
        if(i < inputString.size()){
            valAtIndex = inputString[i];
        }
        else{
            valAtIndex = ' ';
        }

        if (valAtIndex >= '0' && valAtIndex <= '9') {
            tokenNum = tokenNum * 10 + (valAtIndex - '0'); // build the number digit by digit
            inNumber = true;
        } 
        else if (inNumber && (valAtIndex == ' ' || valAtIndex == '\t' || valAtIndex == '\r' || valAtIndex == '\n')) {
            auto vocabItem = vocabSwapped.find(tokenNum);
            if (vocabItem == vocabSwapped.end()) {
                std::cout << "UNKNOWN TOKEN: " + std::to_string(tokenNum) << "\n";
            }
            else {
                output.push_back(vocabItem->second);
            }
            tokenNum = 0;
            inNumber = false;
        }
        else { //fix later cause I want all inputs possible
            std::cout << "UNKNOWN CHARACTER: " << valAtIndex << "\n";
            break;
        }
    }
    //print output :)
    if(useSpecialChars){
        printVectorDirty(output);
    }
    else{
        printVectorClean(output);
    }
    return;
}

void tokenize(){
    std::vector<std::pair<std::string, std::string>> merges; //int might not match up with vocab
    std::unordered_map<std::string,int> vocab; // SWAPPED FROM TOKENIZE DATA STRUCT FOR O(1) READING
    std::ifstream jsonFile("vocab.json");
    std::vector<int> inputAsTokens;
    std::string inputString = "";

    json j = j.parse(jsonFile);

    uint32_t count = 0;
    std::string first;
    std::string second;
    for (const auto& merge : j["merges"]) {
        first = "";
        second = "";

        // nlohmann retrieval shenanigans...
        for (const auto& byte : merge[0]) {
            first.push_back(static_cast<char>(byte.get<unsigned int>()));
        }
        for (const auto& byte : merge[1]) {
            second.push_back(static_cast<char>(byte.get<unsigned int>()));
        }

        merges.emplace_back(first, second);
    }
    merges.emplace_back(first, "<t>"); // end of text manually

    for (const auto& vocabE : j["vocab"]) {
        std::string token;

        for (const auto& byte : vocabE) {
            //static_cast<char>(thing.get<unsigned int>())) byte to char is fancy
            token.push_back(static_cast<char>(byte.get<unsigned int>()));
        }

        vocab[token] = count;
        count++;
    }
    
    // printVector(merges);
    // printMap(vocab);

    std::cout << "Input a phrase to tokenize: \n";

    // I found this line online and it ignores \n to keep from skipping the getline
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::getline(std::cin, inputString);


    // 1. Initial tokens: one string per byte


    std::vector<std::string> tokens;
    for (int i = 0; i < inputString.length(); i++){
        unsigned char byte = static_cast<unsigned char>(inputString[i]);
        std::string token(1, static_cast<char>(byte));
        tokens.push_back(token);
        if(isalnum(static_cast<char>(inputString[i])) && !isalnum(static_cast<char>(inputString[i+1]))){
            tokens.push_back("<w>");
        }
    }
    tokens.push_back("<t>");


    // 2. Replay merges in priority order


    //basically copied and pasted from mergeCorpus, but different because merges is vect pair<string,string> not vect string
    std::vector<std::string> output;

    for (const auto& m : merges) {
        output.clear();

        for (size_t i = 0; i < tokens.size(); ) {

            if (i + 1 < tokens.size() && tokens[i] == m.first && tokens[i+1] == m.second) {
                output.push_back(m.first + m.second);
                i += 2;
            } 
            else {
                output.push_back(tokens[i]);
                i++;
            }
        }
        tokens = std::move(output);
    }


    // 3. strings -> token numbers


    std::vector<int> tokenNums;
    for (const auto& t : tokens) {
        auto vocabItem = vocab.find(t);
        if (vocabItem != vocab.end()) {
            tokenNums.push_back(vocabItem->second);
        } 
        else{
            tokenNums.push_back('!');
        }
    }
    printVector(tokenNums);

    return;
}

void trainTokenizer(){
    const uint8_t CORPUS_BUFFER_SIZE = 50;
    const int MIN_PAIR_COUNT = 5;

    std::vector<std::string> corpus; // values are tokens, can be removed from, added to the end, iterated through.
    std::vector<std::string> vocab; // key with a string
    std::vector<std::pair<std::string, std::string>> merges; // pairs that have a corresponding key

    std::map<std::pair<std::string,std::string>, int> freq; // (first, second) -> count. Rebuilt every pass.
    uint32_t vocabSize = 0;
    char inputNum = 0;
    std::string inputString = "";

    // get file
    std::cout << "Please input your file name:\n";
    std::cin >> inputString;
    std::string fileName = inputString;
    std::ifstream inputFile(fileName);

    if(!inputFile.is_open()){
        std::cout << "404:file was not found\n";
        return;
    }

    std::cout << "Please input a '0' for a vocab size of 1000, and '1' for a vocab size of 32000\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    inputNum = std::getchar();
    if(inputNum == '0'){
        vocabSize = 1000;
    }
    else{
        vocabSize = 32000;
    }


    // 1. INITIALIZE VOCAB first 258 characters as UTF-8

    
    for (unsigned int i = 0; i < 256; i++){
        vocab.push_back(std::string(1, static_cast<char>(i)));
    }
    vocab.push_back("<t>");
    vocab.push_back("<w>");
    

    // 2. MAKE CORPUS


    std::string buffer;
    std::string inputLine;
    std::string inputChar;
    int bufferCount = 0;
    bool endTraining = false;

    while(std::getline(inputFile, inputLine)){
        if (endTraining){
            break;
        }

        buffer += inputLine + '\n';
        bufferCount++;
        // stop conditions
        if (bufferCount < CORPUS_BUFFER_SIZE && inputFile.peek() != EOF){ 
            continue;
        }
        
        for (size_t first = 0; first < buffer.size(); first++) {
            // make second index
            unsigned char second;
            if(first < buffer.size()-1){
                second = buffer[first+1];
            }
            else{
                second = 0;
            }

            // push first
            corpus.push_back(std::string(1, buffer[first]));

            // if current is letter and next is not, after including the current, add end of word.
            if (isalnum(buffer[first]) && !isalnum(second)){
                corpus.push_back("<w>");
            }
        }
        // add on end of file
        if(inputFile.peek() == EOF){
            corpus.push_back("<t>");
        }
        bufferCount = 0;

        // replay merges learned from earlier chunks, so this chunk starts where the last one ended
        for (const auto& merge : merges){
            mergeCorpus(corpus, merge.first, merge.second);
        }


        // 3. BUILD VOCAB


        while (vocab.size() < vocabSize) {
            freq.clear();

            // firstI+1 avoids out of bounds
            for (uint32_t firstI = 0; firstI + 1 < corpus.size(); firstI++) {
                const std::string corpusNext = corpus[firstI + 1];
                // punct or a space strts a token; nothing merges after <w>
                if (isPunct(corpus[firstI][0]) || isPunct(corpusNext[0]) || corpusNext[0] == ' ' || corpus[firstI].back() == '>'){
                    continue;
                }
                //add to freqency as pair
                freq[{corpus[firstI], corpusNext}]++;
            }

            // get most frq pair
            const std::pair<std::string,std::string>* bestPair = nullptr;
            int bestCount = MIN_PAIR_COUNT;

            for (const auto& item : freq) {
                if (item.second > bestCount){ 
                    bestCount = item.second; 
                    bestPair = &item.first; 
                }
            }
            // nothing common left in corpus. Add to corpus.
            if (!bestPair){
                break; //donkey
            }

            vocab.push_back(bestPair->first + bestPair->second);
            merges.push_back(*bestPair); //whole pair to merges

            // clean up corpus
            mergeCorpus(corpus, bestPair->first, bestPair->second);

            // print progress
            if(vocab.size() % 100 == 0){
                std::cout << vocab.size() << " tokens made\n";
            }
        }
        if (vocab.size() >= vocabSize){
            endTraining = true;
        }

        corpus.clear();
        buffer = "";
    }
    inputFile.close();
    //printVector(vocab);


    //PRINT TO JSON

    
    // put vocab in a safe json array
    json jsonForVocab = json::array();
    for (const auto& token : vocab) {
        json bytes = json::array();
        for (unsigned char c : token) {
            bytes.push_back(c);
        }

        jsonForVocab.push_back(bytes);
    }

    //put merges into another safe json array
    json jsonForMerges = json::array();
    for (const auto& merge : merges) {
        json pair = json::array();

        //first ele
        json first = json::array();
        for (unsigned char strAsChar : merge.first) {
            first.push_back(strAsChar);
        }

        //second ele
        json second = json::array();
        for (unsigned char strAsChar : merge.second) {
            second.push_back(strAsChar);
        }

        pair.push_back(first);
        pair.push_back(second);

        jsonForMerges.push_back(pair);
    }

    json j = {
        {"vocab", jsonForVocab},
        {"merges", jsonForMerges}
    };

    std::ofstream file("vocab.json");

    file << j.dump(4);

    file.close();
    std::cout << j.dump(4) << '\n';
    std::cout << "Blistering barnacles! That's the end. -Captain Haddock\n";
    return;
}

int main(void){
    uint8_t inputNum = 0;
    
    // it is time to choose, Mr. Freeman.
    std::cout << "Train tokenizer '0' or tokenize '1' or detokenize '2'\n";
    inputNum = std::getchar();

    if(inputNum == '0'){
        trainTokenizer();
    }
    else if(inputNum == '1'){
        tokenize();
    }
    else if(inputNum == '2'){
        detokenize();
    }
}
