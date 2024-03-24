//
// Created by Armon Barakchi on 3/7/24.
//

#ifndef ECE141_ARCHIVE_CHUNKER_H
#define ECE141_ARCHIVE_CHUNKER_H

#include <cstdio>
#include <cstdint>
#include <functional>
#include <fstream>
#include <cstring>
#include <map>



namespace ECE141 {

    const size_t kBlockSize{2048};
    struct Block;
    struct __attribute__((packed)) MetaData { //51 bytes currently
        char FileName[16];
        uint8_t occupied; //1 is full, 0 is empty
        uint64_t currentBlock;
        uint64_t fileSize;
        uint64_t nextBlock;
        size_t part_num;
        uint8_t processerNumber;
        uint8_t processed; // 1 is processed, 0 is not

    };

    struct Block {
            MetaData Meta;
            char data[kBlockSize-sizeof(MetaData)];
    };




    using BlockVisitor = std::function<bool(Block &aBlock, size_t aPos)>;
    const size_t kDataSize=sizeof(Block)-sizeof(MetaData);
    struct Chunker {


        Chunker(std::istream &anInput, uint32_t aBlockSize) : input(anInput), blockSize(aBlockSize) {

        }

        size_t getSize() const {
            auto curr = input.tellg();
            input.seekg(0, std::ios::end); //end of the stream
            auto ret = input.tellg();
            input.seekg(curr, std::ios::beg);
            return ret;
        }

        size_t chunkCount() const { //how many chunks for given stream?
            auto theSize=getSize();
            return (theSize/kDataSize)+(theSize % kDataSize ? 1: 0);
        }

        bool each(BlockVisitor aCallback) {
            size_t theLen{getSize()};
            input.seekg(0, std::ios::beg); //point to start of input...

            Block   theBlock;
            memset(&theBlock, 0, sizeof(Block));
            size_t  theIndex{0};
            while(theLen && input.good() && !input.eof()) {
                size_t theDelta=std::min(kDataSize, theLen);
                memset(&theBlock, 0, sizeof(Block));
                //std::memset(theBlock.data,0,kDataSize);
                input.read(theBlock.data, theDelta);
                input.clear();
                aCallback(theBlock, theIndex++);
                theLen-=theDelta;
            }
            return true;
        }

    protected:
        std::istream &input;
        uint32_t blockSize;
    };


}




#endif //ECE141_ARCHIVE_CHUNKER_H
