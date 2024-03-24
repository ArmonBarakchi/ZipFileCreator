//
//  Testable.hpp
//
//  Created by rick gessner on 1/8/22.
//

#ifndef Testable_h
#define Testable_h

#include <iostream>
#include <string>
#include <optional>
#include <sstream>

namespace ECE141 {

    class Testable {
    protected:
        size_t count;
    public:

        Testable() : count{0} {}
        using OptString = std::optional<std::string>;
        virtual OptString getTestName(size_t anIndex) const=0;
        virtual bool operator()(const std::string &aName)=0;

        size_t runTests() {
            size_t theCount{0};
            std::stringstream theOutput;
            for(size_t i=0;i<count;i++) {
                if(auto theName=getTestName(i)) {
                    bool theResult=(*this)(theName.value());
                    if(theResult) theCount++;
                    static const char* gResults[]={"FAIL","PASS"};
                    theOutput << i+1 << ". " << theName.value()
                              << ": " << gResults[theResult] << "\n";
                }
            }
            if(theCount!=count) {
                std::cout << theCount << " of " << count;
            }
            else std::cout << "All";
            std::cout << " tests passed.\n" << theOutput.str() << "\n";
            return theCount;
        }

    };

}

#endif /* Testable_h */
