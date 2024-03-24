//
//  Archive.cpp
//
//
//
//

#include "Archive.hpp"

namespace ECE141 {

    std::vector<std::shared_ptr<IDataProcessor>> Archive::processors;

    Archive::Archive(const std::string &aPath, CreateFile aMode, uint32_t _aBlockSize) {
        if (std::string::npos == aPath.find(".arc")) {
            aFullPath = aPath + ".arc";
        } else { aFullPath = aPath; }

        ArchiveFile.open(aFullPath, aMode);
        aBlockSize = _aBlockSize;
    }


    Archive::Archive(const std::string &aPath, OpenFile aMode) {
        if (std::string::npos == aPath.find(".arc")) {
            aFullPath = aPath + ".arc";
        } else { aFullPath = aPath; }

        ArchiveFile.open(aFullPath, aMode);



    }


    Archive::~Archive() {
        ArchiveFile.flush();
        ArchiveFile.close();

    }

    void Archive::notifyObservers(ActionType anAction, const std::string &aName, bool status) {
        for (const auto &observerPtr: observers) {
            (*observerPtr)(anAction, aName, status);
        }
    }

    Archive &Archive::addObserver(std::shared_ptr<ArchiveObserver> anObserver) {
        observers.push_back(anObserver);
        return *this;
    }


    ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName, uint32_t aBlockSize) {
        return ArchiveStatus{std::shared_ptr<Archive>(new Archive(anArchiveName, CreateFile{}, aBlockSize))};
    }

    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        return ArchiveStatus{std::shared_ptr<Archive>(new Archive(anArchiveName, OpenFile{}))};
    }



    ArchiveStatus<bool> Archive::add(const std::string &aFilename, IDataProcessor *aProcessor) {
        //initialize the input

        fs::path thePath(aFilename.c_str());
        std::string theName = thePath.filename();
        std::string theFolderName = findFolderName(aFilename);

        std::fstream theInput(aFilename, OpenFile{}); //used for processing

        //initialize a potential processed input
        std::fstream theProcessedInput;
        theProcessedInput.open(aFullPath + "output.txt.process", CreateFile{});




        //process file if there is a processor
        uint8_t processorPosition;
        std::unique_ptr<ECE141::Chunker> theChunker;
        if (aProcessor) {

            processorPosition = processAnInput(aProcessor, theInput, theProcessedInput, aFilename);
            theChunker = std::make_unique<ECE141::Chunker>(theProcessedInput, aBlockSize);

        } else {
            theChunker = std::make_unique<ECE141::Chunker>(theInput, aBlockSize);
        }

        //find open blocks in archive
        IntVector theOpenPlaces;
        getFreeBlocks(theChunker->chunkCount(), theOpenPlaces);

        //write file to open blocks
        theChunker->each([&](ECE141::Block &aBlock, size_t aPartNum) {

            int j;
            for (int i = 0; i < theFolderName.size(); i++) {
                aBlock.Meta.FileName[i] = theFolderName[i];
                j = i;
            }
            aBlock.Meta.FileName[j + 1] = '\0';

            if (aProcessor) {
                aBlock.Meta.processed = 1;
                aBlock.Meta.processerNumber = processorPosition;
            } else { aBlock.Meta.processed = 0; }


            aBlock.Meta.fileSize = theChunker->getSize();
            aBlock.Meta.occupied = true;
            aBlock.Meta.part_num = aPartNum;
            aBlock.Meta.currentBlock = theOpenPlaces[aPartNum];
            aBlock.Meta.nextBlock = theOpenPlaces[aPartNum + 1];
            writeBlock(aBlock, theOpenPlaces[aPartNum]);
            return true;
        });

        notifyObservers(ActionType::added, theName, true);
        return ArchiveStatus<bool>(true);
    }


    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {

        //initialize an out.txt filestream
        std::fstream outputFile;
        outputFile.open(aFullPath, CreateFile{});
        outputFile.seekp(0, std::ios::beg);


        //initialize loop variables
        int counter{0};
        std::vector<uint8_t> inputVector(kBlockSize, 0);
        size_t dataSize = kBlockSize-sizeof(MetaData);

        //iterate through blocks and add to out.txt
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            std::string tempName(aBlock.Meta.FileName);
            if(size_t apos = tempName.find_last_of('/')) {
                tempName = tempName.substr(apos + 1);
            }
            if (tempName == aFilename) {
                if (true == aBlock.Meta.processed) {
                    if(0 != aBlock.Meta.nextBlock) { //how come inputVector and counter are not recognized in this scope
                        counter++;
                        inputVector.resize(counter*kBlockSize);
                        std::memcpy(inputVector.data() + (counter-1)*dataSize, aBlock.data, dataSize);
                        return true;
                    } else {
                        if(0!=counter){inputVector.resize(counter*kBlockSize);}
                        else{inputVector.resize(kBlockSize);}
                        std::memcpy(inputVector.data() + counter*dataSize, aBlock.data, aBlock.Meta.fileSize-counter*dataSize);
                    }

                    std::vector<uint8_t> outputVector = processors[aBlock.Meta.processerNumber]->reverseProcess(inputVector);
                    outputFile.write(reinterpret_cast<const char*>(outputVector.data()), outputVector.size());

                } else {
                    outputFile.write(aBlock.data, sizeof(aBlock.data));
                }
            }
            return true;
        });

        if(0 != counter) {
            notifyObservers(ActionType::extracted, aFilename, true);
            return ArchiveStatus<bool>(true);
        }
        else {
            notifyObservers(ActionType::extracted, aFilename, false);
            return ArchiveStatus<bool>(false);
        }
    }

    ArchiveStatus<bool> Archive::remove(const std::string &aFilename) {
        int counter{0};
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            std::string tempName(aBlock.Meta.FileName);
            if (aFilename == tempName) {
                counter++;
                aBlock.Meta.occupied = 0;
                writeBlock(aBlock, aBlock.Meta.currentBlock);
            }
            return true;
        });

        if (0 == counter) {
            notifyObservers(ActionType::removed, aFilename, false);
            return ArchiveStatus<bool>(false);
        } else {
            notifyObservers(ActionType::removed, aFilename, true);
            return ArchiveStatus<bool>(true);
        }
    }


    ArchiveStatus<size_t> Archive::list(std::ostream &aStream) {
        std::string header = "###  name         size       date added \n------------------------------------------------\n";
        aStream << header;
        std::string FileNames = " ";
        size_t result{0};

        each([&](Block &aBlock, size_t aPos) { //naive approach...
            std::string tempName(aBlock.Meta.FileName);
            if(size_t apos = tempName.find_last_of('/')) {
                tempName = tempName.substr(apos + 1);
            }
            std::string FileElement;
            if (std::string::npos == FileNames.find(" " + tempName + " ") &&
                1 == aBlock.Meta.occupied) { //this doesn't work because what about
                FileNames += " " + tempName + " ";
                result++;
                std::string tempFileNumber = std::to_string(result);
                std::string tempDataSize = std::to_string(aBlock.Meta.fileSize);
                FileElement = tempFileNumber + ".   " + tempName + "    " + tempDataSize + "\n";
                std::cout << FileElement<<std::endl;
                aStream << FileElement;
            }
            return true;
        });
        notifyObservers(ActionType::listed, "", true);
        return ArchiveStatus<size_t>(result);
    }

    ArchiveStatus<size_t> Archive::debugDump(std::ostream &aStream) {
        size_t result{0};
        std::string header = "###  status   name\n-----------------------\n";
        aStream << header;
        std::string FileElement;
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            std::string BlockNumber = std::to_string(aBlock.Meta.currentBlock);
            std::string status;
            std::string fileName;
            0 == aBlock.Meta.occupied ? status = "empty" : status = "used";
            "used" == status ? fileName = aBlock.Meta.FileName : fileName = "";
            if(size_t apos = fileName.find_last_of('/')) {
                fileName = fileName.substr(apos + 1);
            }
            FileElement = BlockNumber + ".   " + status + "     " + fileName + "\n";
            aStream << FileElement;
            result++;
            return true;
        });

        notifyObservers(ActionType::dumped, "", true);
        return ArchiveStatus<size_t>(result); //gets number of blocks

    }


    ArchiveStatus<size_t> Archive::compact() {
        size_t result{countBlocks()};
        std::fstream tempFile(aFullPath + ".tmp", CreateFile{});
        tempFile.seekp(0, std::ios::beg);
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            if (0 == aBlock.Meta.occupied) {
                result--;
            } else {
                tempFile.write(reinterpret_cast<char *>(&aBlock), kBlockSize);
            }
            return true;
        });

        // delete the old archive
        fs::remove(aFullPath);
        // rename the newfile to aPath
        fs::rename(aFullPath + ".tmp", aFullPath);
        // open the aPath
        ArchiveFile.open(aFullPath, OpenFile{});
        notifyObservers(ActionType::compacted, "", true);
        return ArchiveStatus<size_t>(result);
    }

    ArchiveStatus<std::string> Archive::getFullPath() const {
        return ArchiveStatus<std::string>(aFullPath);
    }

    ArchiveStatus<bool> Archive::resize(size_t _aBlockSize) {
        aBlockSize = _aBlockSize;
        return ArchiveStatus<bool>(true);
    }
    ArchiveStatus<bool> Archive::merge(const std::string &anArchiveName){
        auto newArchive = Archive::openArchive(anArchiveName);
        newArchive.getValue()->addArchive(*this);
        return ArchiveStatus<bool>(true);
    } // New!
    ArchiveStatus<bool> Archive:: addFolder(const std::string &aFolder){
        ECE141::ScanFolder theScan(aFolder);
        theScan.each([this](const fs::directory_entry &anEntry) {
            if(anEntry.is_regular_file()) {
                this->add(anEntry.path());
            }

            return true;
        });


        return ArchiveStatus<bool>(true);
    } // New!
    ArchiveStatus<bool> Archive:: extractFolder(const std::string &aFolderName, const std::string &anExtractPath)
    {
        std::string FileNames = " ";
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            std::string tempName(aBlock.Meta.FileName);
            if (std::string::npos == FileNames.find(" " + tempName + " ") &&
                1 == aBlock.Meta.occupied && std::string::npos != tempName.find(aFolderName)) {
                FileNames += " " + tempName + " ";
                size_t pos = tempName.find('/');
                std::string justFileName = tempName.substr(pos+1);

                extract(justFileName, anExtractPath+justFileName);

            }
            return true;
        });
        return ArchiveStatus<bool>(true);
    }


    //--------------Primitives-------------------//

    ArchiveStatus<bool> Archive::writeBlock(Block &aBlock, size_t aBlockNum) {
        ArchiveFile.seekp(aBlockNum * kBlockSize, std::ios::beg);
        ArchiveFile.write(reinterpret_cast<char *>(&aBlock), kBlockSize);
        ArchiveFile.flush();
        return ArchiveStatus<bool>(true);  //lacks error handling
    }

    size_t Archive::countBlocks() {
        ArchiveFile.clear(); //just in case
        ArchiveFile.seekg(0, std::ios::end);
        return static_cast<size_t>(ArchiveFile.tellg() / kBlockSize);
    }

    ArchiveStatus<bool> Archive::readBlock(Block &aBlock, size_t aBlockNum) {
        ArchiveFile.seekg(aBlockNum * kBlockSize); //go to correct location...
        ArchiveFile.read(reinterpret_cast<char *>(&aBlock), kBlockSize);
        return ArchiveStatus<bool>(true); //lacks error handling
    }

    bool Archive::each(BlockVisitor aVisitor) {
        size_t theCount = countBlocks(), i{0};
        bool more = theCount;
        Block theBlock;

        while (more && theCount > i) {
            readBlock(theBlock, i);
            more = aVisitor(theBlock, i++);
        }
        return more;
    }

    ArchiveStatus<bool> Archive::getFreeBlocks(size_t aCount, IntVector &aList) {
        aList.clear(); //erase previous contents...
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            if (!aBlock.Meta.occupied) {
                aList.push_back(aPos);
            }
            return (aList.size() == aCount); //quit once we have aCount items...
        });
        size_t theBCount = countBlocks();
        while (aList.size() < aCount) {
            aList.push_back(theBCount++);
        }
        aList.push_back(0); //makes linking block logic easier...
        return ArchiveStatus<bool>(true);  //lacks error handling
    }




    //processes inputFile and returns a processor position
    uint8_t Archive::processAnInput(IDataProcessor *aProcessor, std::fstream &anInput, std::fstream &anOutput, const std::string &aFilename) {
        uint8_t processorPosition;
        bool found = false;
        for (int i = 0; i < processors.size(); i++) {
            if (processors[i].get() == aProcessor) {
                processorPosition = i;
                found = true;
            }
        }
        if (!found) {
            std::shared_ptr<IDataProcessor> ProcessorPtr(aProcessor);
            processors.push_back(ProcessorPtr);
            processorPosition = processors.size() - 1;
        }

        std::vector<uint8_t> InputVector(fs::file_size(aFilename));
        anInput.read(reinterpret_cast<char*>(InputVector.data()), fs::file_size(aFilename));

        std::vector<uint8_t> theProcessedVector = aProcessor->process(InputVector);
        anOutput.write(reinterpret_cast<const char*>(theProcessedVector.data()), theProcessedVector.size());

        return processorPosition;

    }

    void Archive::addArchive(Archive &anOriginalArchive) {
        each([&](Block &aBlock, size_t aPos) { //naive approach...
            anOriginalArchive.ArchiveFile.clear();
            anOriginalArchive.ArchiveFile.seekp(0,std::ios::end);
            anOriginalArchive.ArchiveFile.write(reinterpret_cast<char *>(&aBlock), kBlockSize);
            ArchiveFile.flush();
            return true;
        });
    }

    std::string Archive::findFolderName(const std::string &aFilepath) {
        size_t aPos = aFilepath.find_last_of("/\\");

        for(int i = aPos-1; i>0; i--) {
            if(aFilepath[i] == '/') {
                aPos = i;
                break;
            }
        }

        std::string result = aFilepath.substr(aPos+1);
        return result;
    }




    //--------------Compression-------------------------

//    std::vector<uint8_t> ECE141::Compression::process(const std::vector<uint8_t> &input) {
//
//        uLong compressedSize = compressBound(input.size());
//        std::vector<uint8_t> compressedData(compressedSize);
//
//        int result = compress(compressedData.data(), &compressedSize, input.data(), input.size());
//
//        if (result != Z_OK) {
//            return std::vector<uint8_t>();
//        } else {
//            compressedData.resize(compressedSize);
//            return compressedData;
//        }
//
//    }
//
//    size_t const WorstCaseUncompressionSize{8};
//    std::vector<uint8_t> ECE141::Compression::reverseProcess(const std::vector<uint8_t> &input) {
//
//        // Determine the maximum size of the uncompressed data based on the compressed data size
//        size_t uncompressedSize = static_cast<size_t>(input.size()) * WorstCaseUncompressionSize;
//
//        //find a way to figure out size of uncompressedData
//        std::vector<uint8_t> uncompressedData(uncompressedSize, 0 );
//
//        // Decompress the compressed data
//        int result = uncompress(uncompressedData.data(), &uncompressedSize , input.data(), input.size());
//
//        //value of uncompressedSize gets changed by this function to the actual uncompressed size
//        for (unsigned long i = 0; i < uncompressedData.size(); ++i) {
//            std::cout << uncompressedData[i];
//
//        }
//        std::cout << std::endl;
//        // Check the result of decompression
//        if (result != Z_OK) {
//            // Handle error (e.g., return an empty vector)
//            return std::vector<uint8_t>();
//        } else {
//            // Resize the uncompressed data vector to the actual size of the decompressed data
//            uncompressedData.resize(uncompressedSize);
//            //for some reason the uncompressedSize here is 1024??
//            return uncompressedData;
//        }
//    }



}

