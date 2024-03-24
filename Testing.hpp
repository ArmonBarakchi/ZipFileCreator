//
//  Testing.hpp
//
//  Created by rick gessner on 1/24/23.
//
//  Updated on March 14, 2024 - 2:56 PM
//

#ifndef Testing_h
#define Testing_h

#include "Archive.hpp"
#include "Tracker.hpp"
#include "ScanFolder.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <filesystem>
#include <cstring>
#include <unordered_map>

//If you are having trouble with this line make sure you are using C++17
namespace fs = std::filesystem;

namespace ECE141 {

    const size_t kMinBlockSize{1024};
    static std::string kSubFolder{"/sub"};

    struct Testing : public ArchiveObserver {

        //------------------------------------------- The observer interface
        void operator()(ActionType anAction, const std::string& aName, bool status) {
            std::cerr << "observed ";
            switch (anAction) {
                case ActionType::added: std::cerr << "add "; break;
                case ActionType::extracted: std::cerr << "extract "; break;
                case ActionType::removed: std::cerr << "remove "; break;
                case ActionType::listed: std::cerr << "list "; break;
                case ActionType::dumped: std::cerr << "dump "; break;
            }
            std::cerr << aName << "\n";
        }
        //-------------------------------------------
        std::string folder;
        std::unordered_map<std::string, int32_t > stressList;

        std::string getRandomWord() {
            static std::vector<std::string> theWords = {
                    std::string("class"),   std::string("happy"),
                    std::string("coding"),  std::string("pattern"),
                    std::string("design"),  std::string("method"),
                    std::string("dyad"),    std::string("story"),
                    std::string("monad"),   std::string("data"),
                    std::string("compile"), std::string("debug"),
            };
            return theWords[rand() % theWords.size()];
        }

        void makeFile(const std::string& aFullPath, size_t aMaxSize) {
            const char* thePrefix = "";
            std::ofstream theFile(aFullPath.c_str(), std::ios::trunc | std::ios::out);
            size_t theSize = 0;
            size_t theCount = 0;
            while (theSize < aMaxSize) {
                std::string theWord = getRandomWord();
                theFile << thePrefix;
                if (0 == theCount++ % 10) theFile << "\n";
                theFile << theWord;
                thePrefix = ", ";
                theSize += theWord.size() + 2;
            }
            theFile << std::endl;
            theFile.close();
        }

        void buildTestFiles() {
          fs::create_directory(folder+kSubFolder);
        
          makeFile(folder + kSubFolder + "/smallA.txt", 890);
          makeFile(folder + kSubFolder + "/smallB.txt", 890);
          makeFile(folder + kSubFolder + "/mediumA.txt", 1780);
          makeFile(folder + kSubFolder + "/mediumB.txt", 1780);
          makeFile(folder + kSubFolder + "/largeA.txt", 2640);
          makeFile(folder + kSubFolder + "/XlargeA.txt", 264000);
          makeFile(folder + kSubFolder + "/largeB.txt", 2640);
          makeFile(folder + kSubFolder + "/XlargeB.txt", 264000);
        }

        Testing(const std::string& aFolder) : folder(aFolder) {
            buildTestFiles();
        }

        bool doAllTests(std::ostream& anOutput) {
            return doCreateTests(anOutput) &&
                   doOpenTests(anOutput) &&
                   doAddTests(anOutput) &&
                   doExtractTests(anOutput) &&
                   doRemoveTests(anOutput) &&
                   doListTests(anOutput) &&
                   doDumpTests(anOutput) &&
                   doStressTests(anOutput);
        }

        bool doFinalTests(std::ostream& anOutput) {
            return doResizeTests(anOutput) &&
                   doMergeTests(anOutput) &&
                   doFolderTests(anOutput);
        }

        //-------------------------------------------

        bool doCreateTests(std::ostream& anOutput) const {
            ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/test");
            if (theArchive.isOK()) {
                if (!fs::exists(folder + "/test.arc")) { return false; }
            }
            else { return false; }

            ArchiveStatus<std::shared_ptr<Archive>> theArchive2 = Archive::createArchive(folder + "/test");
            if (theArchive2.isOK()) {
                if (!fs::exists(folder + "/test.arc")) { return false; }
            }
            else { return false; }

            ArchiveStatus<std::shared_ptr<Archive>> theArchive3 = Archive::createArchive(folder + "/test2");
            if (theArchive3.isOK()) {
                if (!fs::exists(folder + "/test2.arc")) { return false; }
            }
            else { return false; }
            return true;
        }

        //-------------------------------------------

        bool doOpenTests(std::ostream& anOutput) {
            { // now scoped to limit theArchive's scope...
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/test");
                if (!theArchive.isOK()) {
                    anOutput << "Failed to create archive\n";
                    return false;
                }
                if (!theArchive.getValue()->add(folder + kSubFolder +  "/smallA.txt").isOK()) {
                    anOutput << "Failed to add file\n";
                    return false;
                }
            }
            {
                ArchiveStatus<std::shared_ptr<Archive>> newArchive = Archive::openArchive(folder + "/test");
                if (!newArchive.isOK()) {
                    anOutput << "Failed to open archive\n";
                    return false;
                }
                std::fstream testFileStream;
                testFileStream.open(folder + "/test.arc");
                if (testFileStream.fail()) {
                    anOutput << "Failed to open archive file. Something wrong with the archive file created\n";
                    return false;
                }
            }
            return true;
        }

        size_t addTestFile(Archive& anArchive,
                           const std::string& aName, char aChar = 'A',
                           IDataProcessor* aProcessor = nullptr) {
            std::string theFullPath(folder + kSubFolder + "/" + aName + aChar + ".txt");
            anArchive.add(theFullPath, aProcessor);
            return 1;
        }

        size_t addTestFiles(Archive& anArchive, char aChar = 'A',
                            IDataProcessor* aProcessor = nullptr) {
            addTestFile(anArchive, "small", aChar, aProcessor);
            addTestFile(anArchive, "medium", aChar, aProcessor);
            addTestFile(anArchive, "large", aChar, aProcessor);
            addTestFile(anArchive, "Xlarge", aChar, aProcessor);
            return 4;
        }

        size_t getFileSize(const std::string& aFilePath) {
            std::ifstream theStream(aFilePath, std::ios::binary);
            const auto theBegin = theStream.tellg();
            theStream.seekg(0, std::ios::end);
            const auto theEnd = theStream.tellg();
            return theEnd - theBegin;
        }

        bool hasMinSize(const std::string& aFilePath, size_t aMinSize) {
            size_t theSize = getFileSize(aFilePath);
            return theSize >= aMinSize;
        }

        bool hasMaxSize(const std::string& aFilePath, size_t aMinSize) {
            size_t theSize = getFileSize(aFilePath);
            return theSize <= aMinSize;
        }

        size_t countLines(const std::string &anOutput) {
            std::stringstream theOutput(anOutput);
            std::string theLine;
            size_t theCount{0};

            while(getline(theOutput,theLine)) {
                std::cout << theLine << "\n";
                theCount++;
            }
            return theCount;
        }

        size_t countLinesContainingSubstring(const std::string &anOutput, const std::string &aSubstring) {
            std::stringstream theOutput(anOutput);
            std::string theLine;
            size_t theCount = 0;

            while (getline(theOutput, theLine)) {
                std::cout << theLine << "\n";
                theCount += static_cast<size_t>(theLine.find(aSubstring) != std::string::npos);
            }

            return theCount;
        }

        bool doAddTests(std::ostream& anOutput, char aChar = 'A') {
            bool    theResult = false;
            size_t  theCount = 0;
            size_t  theAddCount = 0;

            std::string theFullPath(folder+"/addtest"+aChar+".arc");
            { // block to limit scope of theArchive...
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(theFullPath);
                if (theArchive.isOK()) {
                    std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                    theArchive.getValue()->addObserver(theObserver);
                    theAddCount = addTestFiles(*theArchive.getValue(), aChar);
                }
                else{
                    anOutput << "Failed to create archive\n";
                    return false;
                }
            }
            {
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(theFullPath);
                if(theArchive.isOK()) {
                    std::stringstream theStream;
                    theArchive.getValue()->list(theStream);
                    theCount=countLines(theStream.str());
                    theCount--; //remove the header line...
                    theCount--; //remove the --------------
                }
                else{
                    anOutput << "Failed to open archive\n";
                    return false;
                }
            }
            if (theCount == 0 || theCount != theAddCount) {
                anOutput << "Archive doesn't have enough elements\n";
                anOutput << "Expected " << theAddCount << " but got " << theCount << "\n";
                anOutput << "Either the add failed or the list failed\n";
                return false;
            }
            else {
                theResult = hasMinSize(theFullPath, 270144);
                if(!theResult) {
                    anOutput << "Archive is too small.\n";
                    anOutput << "Expected at least 270144 bytes, but got " << getFileSize(theFullPath) << "\n";
                }
            }

            return theResult;
        }

        //-------------------------------------------

        //why is large.text rather than large.txt?
        std::string pickRandomFile(char aChar='A') {
            static const char* theFiles[] = { "small","medium","large" };
            size_t theCount = sizeof(theFiles) / sizeof(char*);
            const char* theName=theFiles[rand() % theCount];
            std::string theResult(theName);
            theResult+=aChar;
            theResult+=".txt";
            return theResult;
        }

        bool filesMatch(const std::string& aFilename, const std::string& aFullPath) {
            std::string theFilePath(folder+kSubFolder+"/"+aFilename);
            std::ifstream theFile1(theFilePath);
            std::ifstream theFile2(aFullPath);

            if (!theFile1.is_open() || !theFile2.is_open()) {
                fs::path thePath(aFullPath);
                std::cerr << "File '" << thePath.filename().string() << "' does NOT exist\n";
                return false;
            }

            std::string theLine1;
            std::string theLine2;

            bool theResult{true};
            while (theResult && std::getline(theFile1, theLine1)) {
                std::getline(theFile2, theLine2);
                theResult=theLine1==theLine2;
                if(!theResult){
                    std::cerr << "[DEBUG] Files Match funtion failed because of a mismatch at <<: \n"
                    << theLine1 << "\n != \n"
                    << theLine2 << "\n";
                }
            }
            return theResult;
        }

        bool doExtractTests(std::ostream& anOutput) {
            auto& theTracker = Tracker::instance();
            theTracker.enable(true).reset();
            {
                bool theResult = false;
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/extracttest");
                if (theArchive.isOK()) {
                    std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                    theArchive.getValue()->addObserver(theObserver);
                    addTestFiles(*theArchive.getValue());

                    std::string theFileName = pickRandomFile();
                    std::string temp(folder + "/out.txt");
                    theArchive.getValue()->extract(theFileName, temp);
                    theResult = filesMatch(theFileName, temp);

                    if (!theResult) {
                        anOutput << "Extracted file doesn't match original.\n";
                        return theResult;
                    }
                }
                else{
                    anOutput << "Failed to create archive for the extract test\n";
                    return false;
                }
            }
            theTracker.reportLeaks(anOutput);
            return true;
        }

        //-------------------------------------------

        bool verifyRemove(const std::string& aName, size_t aCount, std::string& anOutput) {
            std::stringstream theInput(anOutput);
            std::string theName;
            std::string line;
            size_t theCount=0;

            while (!theInput.eof()) {
                std::getline(theInput, line);
                // check if the line is not empty and not a header or a separator
                if (!line.empty() && line[0]!='-' && line[0]!='#') {
                    std::stringstream theLineInput(line);
                    theLineInput >> theName >> theName;
                    if (theName == aName) {
                        std::cerr << "Found " << aName << " in the list but it should have been deleted\n";
                        return false;
                    }
                    theCount++;
                }
            }

            return aCount==theCount;
        }

        bool doRemoveTests(std::ostream& anOutput) {
            bool theResult = false;
            std::string theFileName;
            {
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/test");
                if (theArchive.isOK()) {
                    std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                    theArchive.getValue()->addObserver(theObserver);
                    addTestFiles(*theArchive.getValue());
                    theFileName = pickRandomFile();
                    theArchive.getValue()->remove(theFileName);
                }
                else{
                    anOutput << "Failed to create archive\n";
                    return false;
                }
            }

            {
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(folder + "/test");
                if(theArchive.isOK()) {
                    std::stringstream theStream;
                    theArchive.getValue()->list(theStream);
                    std::string theOutput = theStream.str();
                    theResult = verifyRemove(theFileName, 3, theOutput);
                    anOutput << theOutput;
                    if(!theResult) anOutput << "remove file failed\n";
                }
                else{
                    anOutput << "Failed to open archive\n";
                    return false;
                }
            }

            return theResult;
        }

        //-------------------------------------------

        bool verifyArchiveAgainstAMap(const std::string& aString, std::unordered_map<std::string, int32_t> theCounts) const {
            std::stringstream theInput(aString);
            std::string line;
            while (!theInput.eof()) {
                std::getline(theInput, line);
                std::stringstream theLineInput(line);
                std::string theName;
                while (theLineInput >> theName) {
                    if (theCounts.count(theName)) {
                        theCounts[theName] -= 1;
                    }
                }
            }
            bool theResult = true;
            // check if all the counts are zero
            for (const auto& [prefix, count] : theCounts) {
                theResult &= count == 0;
            }
            return theResult;
        }

        bool doListTests(std::ostream& anOutput) {
            bool theResult = false;
            { // now scoped to limit theArchive's scope...
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/test");
                if (theArchive.isOK()) {
                    std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                    theArchive.getValue()->addObserver(theObserver);
                    addTestFiles(*theArchive.getValue());
                    addTestFiles(*theArchive.getValue(), 'B');
                } else {
                    anOutput << "Failed to create archive\n";
                    return false;
                }
            }
            { // now scoped to limit theArchive's scope...
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(folder + "/test");
                std::stringstream theStream;
                theArchive.getValue()->list(theStream);
                std::string theOutput = theStream.str();
                theResult = verifyArchiveAgainstAMap(theOutput, {
                        {"smallA.txt", 1},
                        {"mediumA.txt", 1},
                        {"largeA.txt", 1},
                        {"XlargeA.txt", 1},
                        {"smallB.txt", 1},
                        {"mediumB.txt", 1},
                        {"largeB.txt", 1},
                        {"XlargeB.txt", 1},
                });
                if(!theResult) {
                    anOutput << "lists didn't match! what was expected for ";
                    anOutput << "A set and B set insertion with small, medium, large and xlarge\n";
                    return false;
                }
            }
            return theResult;
        }

        bool doDumpTests(std::ostream& anOutput) {
            bool theResult = false;
            auto& theTracker = Tracker::instance();
            theTracker.enable(true).reset();
            {
                {
                    ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(folder + "/dumptest");
                    if (theArchive.isOK()) {
                        std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                        theArchive.getValue()->addObserver(theObserver);
                        addTestFiles(*theArchive.getValue(), 'A');
                        addTestFiles(*theArchive.getValue(), 'B');
                        std::stringstream theStream;
                        auto debugCount = theArchive.getValue()->debugDump(theStream).getValue();
                        // we should have at minimum 530 lines in the debug dump and at most 592 blocks
                        if (530 <= debugCount && debugCount <= 592) {
                            theResult = verifyArchiveAgainstAMap(theStream.str(), {
                                    {"smallA.txt", 1},
                                    {"mediumA.txt", 2},
                                    {"largeA.txt", 3},
                                    {"smallB.txt", 1},
                                    {"mediumB.txt", 2},
                                    {"largeB.txt", 3},
                            });
                        }
                        anOutput << theStream.str();
                    }
                    else{
                        anOutput << "Failed to create archive\n";
                        return false;
                    }
                }

                if (theResult) {
                    std::string theArcName(folder + "/dumptest.arc");
                    theResult = hasMinSize(theArcName, 1024 * 530);
                    if (!theResult) {
                        anOutput << "Archive is too small.\n";
                        anOutput << "Expected at least 1024 * 530 bytes, but got " << getFileSize(theArcName) << "\n";
                    }
                }
            }
            theTracker.reportLeaks(anOutput);
            return theResult;
        }

        //-------------------------------------------

        bool stressVerify(Archive& anArchive){
            std::stringstream listOutput;
            anArchive.list(listOutput);
            std::string theOutput = listOutput.str();
            return verifyArchiveAgainstAMap(theOutput, stressList);
        }

        //-------------------------------------------

        bool stressAdd(Archive& anArchive) {
            static int counter = 0;
            std::stringstream temp;
            temp << "fake" << ++counter << ".txt";
            std::string theName(temp.str());
            stressList[theName] = 1;
            const int theMin = 1000;
            size_t theSize = theMin + rand() % ((2001) - theMin);
            std::string theFullPath(folder + kSubFolder + "/" + theName);
            makeFile(theFullPath, theSize);
            anArchive.add(theFullPath);
            return stressVerify(anArchive);
        }

        //-------------------------------------------

        bool stressRemove(Archive& anArchive) {
            if (!stressList.empty()) {
                auto it = stressList.begin();
                std::advance(it, rand() % stressList.size());
                std::string theName = it->first;
                stressList.erase(theName);
                anArchive.remove(theName);
                return stressVerify(anArchive);
            }
            return true;
        }

        //-------------------------------------------

        bool stressExtract(Archive& anArchive) {
            if (!stressList.empty()) {
                std::string theOutFileName(folder + "/out.txt");
                auto it = stressList.begin();
                std::advance(it, rand() % stressList.size());
                std::string theName = it->first;
                anArchive.extract(theName, theOutFileName);
                return filesMatch(theName, theOutFileName);
            }
            return true;
        }

        //-------------------------------------------
        //return block count...
        size_t doStressDump(Archive& anArchive, size_t& aFreeCount) {
            size_t theBlockCount = 0;
            if (stressList.size()) {
                std::stringstream theOutput;
                anArchive.debugDump(theOutput);

                std::stringstream theInput(theOutput.str());
                std::string line;
                std::string theStatus;
                std::string theName;

                while (!theInput.eof()) {
                    std::getline(theInput, line);
                    if (!line.empty()) {
                        std::stringstream theLineInput(line);
                        theLineInput >> theName >> theStatus >> theName;

                        if (theStatus == "empty") {
                            aFreeCount++;
                            theBlockCount++;
                        }
                        else if (stressList.count(theName)) {
                            theBlockCount++;
                        }
                    }
                }
            }
            return theBlockCount;
        }

        //-------------------------------------------

        bool doStressTests(std::ostream& anOutput) {
            bool theResult = true;

            std::string thePath(folder + "/stresstest.arc");
            if (ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(thePath); theArchive.getValue()) {
                std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                theArchive.getValue()->addObserver(theObserver);
                addTestFiles(*theArchive.getValue(), 'B'); //bootstrap...
                stressList["smallB.txt"] = 1;
                stressList["mediumB.txt"] = 1;
                stressList["largeB.txt"] = 1;
                stressList["XlargeB.txt"] = 1;

                size_t theOpCount = 500;
                static ActionType theCalls[] = {
                        ActionType::added,
                        ActionType::removed,
                        ActionType::extracted
                };

                while (theResult && theOpCount--) {
                    switch (theCalls[rand() % 3]) {
                        case ActionType::added:
                            theResult = stressAdd(*theArchive.getValue());
                            break;
                        case ActionType::removed:
                            theResult = stressRemove(*theArchive.getValue());
                            break;
                        case ActionType::extracted:
                            theResult = stressExtract(*theArchive.getValue());
                            break;
                        default:
                            break;
                    }
                } //while there are operations to do random operations...
            }

            if (theResult) {

                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(thePath);

                size_t thePreFreeCount = 0;
                size_t thePreCount = doStressDump(*theArchive.getValue(), thePreFreeCount);
                size_t thePreSize = getFileSize(thePath);

                //Final test: Dump, compact, re-dump and compare...
                theResult=false;
                if (theArchive.isOK() && thePreCount && thePreSize) {

                    if (auto theBlockCount = theArchive.getValue()->compact(); theBlockCount.getValue()) {
                        size_t thePostFreeCount = 0;
                        size_t thePostCount = doStressDump(*theArchive.getValue(), thePostFreeCount);
                        theResult = (thePostCount <= thePreCount) && (thePostFreeCount <= thePreFreeCount);
                    }

                    if (theResult) { //compacted file should be smaller...
                        size_t thePostSize = getFileSize(thePath);
                        theResult = thePostSize <= thePreSize;
                    }
                }
            } //if

            return theResult;
        }


        bool doCompressTests(std::ostream& anOutput) {
            bool theResult = false;
            std::string theFullPath(folder + "/compressTest.arc");
            size_t  theCount = 0;
            size_t  theAddCount = 0;

            { // block to limit scope of theArchive...
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::createArchive(theFullPath);
                if (theArchive.isOK()) {
                  std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                  IDataProcessor* theProcessor = new Compression();
                  theArchive.getValue()->addObserver(theObserver);
                  addTestFile(*theArchive.getValue(), "Xlarge", 'B', theProcessor);
                  addTestFile(*theArchive.getValue(), "small", 'A', theProcessor);
                  addTestFile(*theArchive.getValue(), "Xlarge", 'A', theProcessor);
                  addTestFile(*theArchive.getValue(), "medium", 'A', theProcessor);
                  addTestFile(*theArchive.getValue(), "small", 'B');
                  addTestFile(*theArchive.getValue(), "large", 'A' ,theProcessor);
                  addTestFile(*theArchive.getValue(), "medium", 'B', theProcessor);
                  addTestFile(*theArchive.getValue(), "large", 'B' );
                  theAddCount = 8;
                }
                else{
                    anOutput << "Failed to create archive\n";
                    return false;
                }
            }
            {
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(theFullPath);
                if(theArchive.isOK()) {
                    std::stringstream theStream;
                    theArchive.getValue()->list(theStream);
                    theCount=countLines(theStream.str());
                    theCount--; //remove the header line...
                    theCount--; //remove the --------------
                }
                else{
                    anOutput << "Failed to open archive\n";
                    return false;
                }
            }
            if (theCount == 0 || theCount != theAddCount) {
                anOutput << "Archive doesn't have enough elements\n";
                return false;
            }
            else {
                theResult = hasMaxSize(theFullPath, 540288/2);
                if(!theResult) {
                    anOutput << "Archive is too large should be at least compressed ot half size\n";
                    return false;
                }
            }
            // now extract all files and compare to originals
            {
                ArchiveStatus<std::shared_ptr<Archive>> theArchive = Archive::openArchive(theFullPath);
                if (theArchive.isOK()) {
                    std::stringstream theStream;
                    std::string theFileName = pickRandomFile();
                    std::string temp(folder + "/out.txt");
                    theArchive.getValue()->extract(theFileName, temp);
                    theResult = filesMatch(theFileName, temp);
                    if(!theResult) {
                        anOutput << "Extracted file do not match original\n";
                        return theResult;
                    }
                }
            }

            return theResult;
        }
        
        //-------------------------------------------

      bool doMergeTests(std::ostream& anOutput) {
        std::stringstream theOutput;
        std::string thePathA(folder+"/addtestA.arc");
        std::string thePathB(folder+"/addtestB.arc");
        bool theResult=doAddTests(theOutput, 'A');

        if(theResult) {
          theResult=doAddTests(theOutput, 'B');
          if(theResult) {
            auto theArchive = Archive::openArchive(thePathA);
            auto theResultB=theArchive.getValue()->merge(thePathB);
            theResult=theResultB.getValue();
          }
        }
        
        if(theResult) {
          auto theArchive = Archive::openArchive(thePathA);
          if(theArchive.isOK()) {
            std::stringstream theStream;
            theArchive.getValue()->merge(thePathB);
            constexpr size_t theAddCount{8};
            theArchive.getValue()->list(theStream);
            auto theCount=countLines(theStream.str());  // handle -2 for header and footer
            theCount--; //remove the header line...
            theCount--; //remove the --------------
            if (theCount == 0 || theCount != theAddCount) {
              anOutput << "Archive doesn't have enough elements\n";
              theResult=false;
            }
            else {
              constexpr size_t kMinSize{265000};
              theResult = hasMinSize(thePathA, kMinSize);
              if(theResult) {
                std::string theFileName = pickRandomFile('B');
                std::string temp(folder + "/out.txt");
                theArchive.getValue()->extract(theFileName, temp);
                theResult = filesMatch(theFileName, temp);
                if (!theResult) {
                  anOutput << "Extracted file doesn't match original.\n";
                }
              }
              else anOutput << "Archive is too small\n";
            }
          }
          else{
            anOutput << "Failed to open archive\n";
            theResult=false;
          }
        }
        else anOutput << "Merge failed\n";
        
        return theResult;
      }

        //-------------------------------------------

        /**
         * New test for final. Will ask to resize an archives block size
         * @param anOutput
         * @return
         */
        bool doResizeTests(std::ostream& anOutput){
            std::stringstream theOutput;
            std::string thePath(folder+"/resizetest.arc");
            bool theResult;
            {
                auto archive = Archive::createArchive(thePath, 2048);
                if (!archive.isOK()) {
                    anOutput << "Failed to create archive, we tried to create an archive with size 2048\n";
                    return false;
                }
                theResult = addTestFiles(*archive.getValue(), 'A');
                if  (!theResult){
                    anOutput << "Add failed for A files, we were trying to add A files when block size is 2048\n";
                    return theResult;
                }
                theResult = addTestFiles(*archive.getValue(), 'B');
                if  (!theResult){
                    anOutput << "Add failed for B files, we were trying to add B files when block size is 2048\n";
                    return theResult;
                }
            }
            {
                auto theArchive = Archive::openArchive(thePath);
                if (!theArchive.isOK()) {
                    anOutput << "Failed to open archive\n";
                    anOutput << "\033[1;31m WARNING: If you see this your open might not be handling updated size\033[0m";
                    return false;
                }
                std::stringstream theStream;
                theArchive.getValue()->list(theStream);
                theResult = verifyArchiveAgainstAMap(theStream.str(), {
                        {"smallA.txt",  1},
                        {"mediumA.txt", 1},
                        {"largeA.txt",  1},
                        {"XlargeA.txt", 1},
                        {"smallB.txt",  1},
                        {"mediumB.txt", 1},
                        {"largeB.txt",  1},
                        {"XlargeB.txt", 1},
                });
                if (!theResult) {
                    anOutput << "lists didn't match! what was expected for ";
                    anOutput << "A set and B set insertion with small, medium, large and xlarge\n";
                    return theResult;
                }
                theStream.str("");
                theArchive.getValue()->debugDump(theStream);
                theResult = verifyArchiveAgainstAMap(theStream.str(), {
                        {"smallA.txt",  1},
                        {"mediumA.txt", 1},
                        {"largeA.txt",  2},
                        {"smallB.txt",  1},
                        {"mediumB.txt", 1},
                        {"largeB.txt",  2},
                });
                if (!theResult) {
                    anOutput << "dump didn't match! what was expected for ";
                    anOutput << "A set and B set insertion with small, medium, large and xlarge\n";
                    anOutput << "\033[1;31m WARNING: Go into verifyArchiveAgainstMap funciton and see which count is not 0.\n"
                               "Make sure calculations check out. If you believe there is an issue in this stage see a proctor\033[0m";
                    return theResult;
                }
            }
            anOutput << "Congrats at this point you have you were able to verify new block size "
                    << "lets now check if extraction still works\n";
            auto theArchive = Archive::openArchive(thePath);
            std::string theFileName = pickRandomFile();
            std::string temp(folder + "/out.txt");
            theArchive.getValue()->extract(theFileName, temp);
            theResult = filesMatch(theFileName, temp);
            if(!theResult) {
                anOutput << "Extracted file do not match original\n";
                return theResult;
            }
            std::cout << "Extraction still works" <<std::endl;
            return theResult;
        }

        size_t countFilesInFolder(const std::string& aPath){
            size_t theCount = 0;
            ScanFolder theScanner(aPath);
            theScanner.each([&](const fs::directory_entry&) {
                ++theCount;
                return true;
            });

            return theCount;
        }

        bool doFolderTests(std::ostream& anOutput){
            const std::string theFullPath = folder + "/foldertest.arc";
            const std::string theFolderPath = folder + kSubFolder + "/";

            // Add folder
            {
                auto theStatus = Archive::createArchive(theFullPath);
                if (!theStatus.isOK()) {
                    anOutput << "Failed to create archive\n";
                    return false;
                }
                auto theArchive = theStatus.getValue();

                std::shared_ptr<ArchiveObserver> theObserver = std::make_shared<Testing>(*this);
                theArchive->addObserver(theObserver);
                theArchive->addFolder(theFolderPath);
            }

            // Verify file count
            {
                auto theStatus = Archive::openArchive(theFullPath);
                if (!theStatus.isOK()) {
                    anOutput << "Failed to open archive\n";
                    return false;
                }
                auto theArchive = theStatus.getValue();

                std::stringstream theStream;
                theArchive->list(theStream);
                const size_t theLineCount = countLinesContainingSubstring(theStream.str(), ".txt");
                const size_t theFileCount = countFilesInFolder(theFolderPath);
                std::cout << "file count: " << theFileCount << "\n";

                if (theLineCount != theFileCount) {
                    anOutput << "Not enough items in archive: expected " << theFileCount << ", got " << theLineCount << "\n";
                    return false;
                }
                std::cout <<"add Folder worked" <<std::endl;
            }

            // Extract folder
            {
                auto theStatus = Archive::openArchive(theFullPath);
                if (!theStatus.isOK()) {
                    anOutput << "Failed to open archive\n";
                    return false;
                }
                auto theArchive = theStatus.getValue();

                const std::string theOutputFolderPath = folder + "/folder_out/";
                fs::create_directory(theOutputFolderPath);
                std::string subFolder = kSubFolder;
                subFolder = subFolder.substr(1, subFolder.length());
                theArchive->extractFolder(subFolder, theOutputFolderPath);

                bool doFilesMatch = true;
                ScanFolder theScanner(theFolderPath);
                theScanner.each([&](const fs::directory_entry& aDirectory) {
                    const auto theFilename = aDirectory.path().filename().string();
                    doFilesMatch = filesMatch(theFilename, theOutputFolderPath + theFilename);

                    if (!doFilesMatch) {
                        anOutput << "File '" << theFilename << "' does not match.";
                        return false;
                    }

                    return true;
                });

                if (!doFilesMatch)
                    return false;
            }

            return true;
        }
    };


}

#endif /* Testing_h */
