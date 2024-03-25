//
//  main.cpp
//

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <functional>
#include <string>
#include <map>
#include "Testing.hpp"

std::string getLocalFolder() {
    //return std::string("c:/xxx/yyy");

    return std::string("/Users/armonbarakchi/Desktop/ZipFileCreator/tmp"); //SET PATH TO LOCAL ARCHIVE FOLDER, add a tmp folder at the end

}

void runAutoGrader(int argc, const char* argv[]) {
    srand(time(nullptr));

    std::string temp(argv[1]);
    std::stringstream theOutput;

    std::string theFolder(getLocalFolder());
    if (3 == argc)
        theFolder = argv[2];
    ZipFileCreator::Testing theTester(theFolder);

    using TestCall = std::function<bool()>;
    static std::map<std::string, TestCall> theCalls{
                {"Compile",  [&]() { return true; }},
                {"Create",   [&]() { return theTester.doCreateTests(theOutput); }},
                {"Open",     [&]() { return theTester.doOpenTests(theOutput); }},
                {"Add",      [&]() { return theTester.doAddTests(theOutput); }},
                {"Extract",  [&]() { return theTester.doExtractTests(theOutput); }},
                {"Remove",   [&]() { return theTester.doRemoveTests(theOutput); }},
                {"List",     [&]() { return theTester.doListTests(theOutput); }},
                {"Merge",    [&]() { return theTester.doMergeTests(theOutput);}  },
                {"Dump",     [&]() { return theTester.doDumpTests(theOutput); }},
                {"Stress",   [&]() { return theTester.doStressTests(theOutput); }},
                {"Compress", [&]() { return theTester.doCompressTests(theOutput); }},
                {"Folder",   [&]() { return theTester.doFolderTests(theOutput); }},
                {"All",      [&]() { return theTester.doAllTests(theOutput); }},
                {"Final",    [&]() { return theTester.doFinalTests(theOutput); }} // All the final exam tests
            };

    std::string theCmd(argv[1]);
    if (theCalls.count(theCmd)) {
        bool theResult = theCalls[theCmd]();
        const char* theStatus[] = {"FAIL", "PASS"};
        std::cout << theCmd << " test " << theStatus[theResult] << "\n";
        std::cout << "------------------------------\n"
            << theOutput.str() << "\n";
    }
    else
        std::cout << "Unknown test\n";
}

int main(int argc, const char* argv[]) {
    if (argc > 1)
        runAutoGrader(argc, argv);


    return 0;
}
