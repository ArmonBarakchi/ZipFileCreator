//
//  Archive.hpp
//
//
//
//

#ifndef Archive_hpp
#define Archive_hpp

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <optional>
#include <stdexcept>
#include <fstream>
#include <map>
#include <sstream>
#include <filesystem>
#include "Chunker.h"
#include <cmath>
#include <zlib.h>
#include <functional>
#include <cstring>
#include "ScanFolder.hpp"

namespace ECE141 {

    enum class ActionType {added, extracted, removed, listed, dumped, compacted};
    namespace fs = std::filesystem;
    class Archive; //forward declaration

    struct ArchiveObserver {
        void virtual operator()(ActionType anAction,
                        const std::string &aName, bool status) =0;
    }; //used to create observers if needed in the future

    //--------------------------------------------

    class IDataProcessor {
    public:
        virtual std::vector<uint8_t> process(const std::vector<uint8_t>& input) = 0;
        virtual std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) = 0;
        virtual ~IDataProcessor(){};
    };


    class Compression : public IDataProcessor {
    public:
        std::vector<uint8_t> process(const std::vector<uint8_t>& input) override;
        std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) override;
        ~Compression() override = default;
    };

    //--------------------------------------------

    enum class ArchiveErrors {
        noError=0,
        fileNotFound=1, fileExists, fileOpenError, fileReadError, fileWriteError, fileCloseError,
        fileSeekError, fileTellError, fileError, badFilename, badPath, badData, badBlock, badArchive,
        badAction, badMode, badProcessor, badBlockType, badBlockCount, badBlockIndex, badBlockData,
        badBlockHash, badBlockNumber, badBlockLength, badBlockDataLength, badBlockTypeLength
    };

    template<typename T>
    class ArchiveStatus {
    public:
        // Constructor for success case
        explicit ArchiveStatus(const T value)
                : value(value), error(ArchiveErrors::noError) {}

        // Constructor for error case
        explicit ArchiveStatus(ArchiveErrors anError)
                : value(std::nullopt), error(anError) {
            if (anError == ArchiveErrors::noError) {
                throw std::logic_error("Cannot use noError with error constructor");
            }
        }

        // Deleted copy constructor and copy assignment to make ArchiveStatus move-only
        ArchiveStatus(const ArchiveStatus&) = delete;
        ArchiveStatus& operator=(const ArchiveStatus&) = delete;

        // Default move constructor and move assignment
        ArchiveStatus(ArchiveStatus&&) noexcept = default;
        ArchiveStatus& operator=(ArchiveStatus&&) noexcept = default;

        T getValue() const {
            if (!isOK()) {
                throw std::runtime_error("Operation failed with error");
            }
            return *value;
        }

        bool isOK() const { return error == ArchiveErrors::noError && value.has_value(); }
        ArchiveErrors getError() const { return error; }

    private:
        std::optional<T> value;
        ArchiveErrors error;
    };

    //--------------------------------------------

    struct CreateFile { //auto-generate proper mode flags to create
        operator std::ios_base::openmode() {
            return std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc;
        }
    };

    struct OpenFile { //auto-generate correct mode flags to open
        operator std::ios_base::openmode() {
            return std::ios::binary | std::ios::in | std::ios::out;
        }
    };

    //------------------------------------------

    class Archive {
    public:
        using IntVector = std::vector<size_t>;
        ~Archive();

        static    ArchiveStatus<std::shared_ptr<Archive>> createArchive(const std::string &anArchiveName, uint32_t aBlockSize=1024);
        static    ArchiveStatus<std::shared_ptr<Archive>> openArchive(const std::string &anArchiveName);

        Archive&  addObserver(std::shared_ptr<ArchiveObserver> anObserver);

        ArchiveStatus<bool>      add(const std::string &aFilename, IDataProcessor* aProcessor=nullptr);
        ArchiveStatus<bool>      extract(const std::string &aFilename, const std::string &aFullPath);
        ArchiveStatus<bool>      remove(const std::string &aFilename);


        ArchiveStatus<bool>      merge(const std::string &anArchiveName);
        ArchiveStatus<bool>      addFolder(const std::string &aFolder);
        ArchiveStatus<bool>      extractFolder(const std::string &aFolderName, const std::string &anExtractPath); // New!

        ArchiveStatus<size_t>    list(std::ostream &aStream);
        ArchiveStatus<size_t>    debugDump(std::ostream &aStream);

        ArchiveStatus<size_t>    compact();
        ArchiveStatus<std::string> getFullPath() const; //get archive path (including .arc extension)

    protected:
        //---------Data Members and Constructors--------

        static std::vector<std::shared_ptr<IDataProcessor>> processors;
        std::vector<std::shared_ptr<ArchiveObserver>> observers;
        Archive(const std::string &aPath, CreateFile aCreate, uint32_t aBlockSize);
        Archive(const std::string &aPath, OpenFile anOpen);
        std::fstream ArchiveFile;
        std::string aFullPath;
        uint32_t aBlockSize;

        //----------Primitives------------------

        void notifyObservers(ActionType anAction, const std::string &aName, bool status);
        size_t countBlocks();
        ArchiveStatus<bool> getFreeBlocks(size_t aCount, IntVector &aList);
        bool each(BlockVisitor aVisitor);
        ArchiveStatus<bool> readBlock(Block &aBlock, size_t aBlockNum);
        ArchiveStatus<bool> writeBlock(Block &aBlock, size_t aBlockNum);
        uint8_t processAnInput(IDataProcessor *aProcessor, std::fstream &anInput, std::fstream &anOutput, const std::string &aFilename);
        void addArchive(Archive &anOriginalArchive);
        std::string findFolderName(const std::string &aFilepath);

    };

}

#endif /* Archive_hpp */