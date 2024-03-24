//
//  Tracker.hpp
//  memtest
//
//  Created by rick gessner on 1/22/22.
//

#ifndef Tracker_h
#define Tracker_h

#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

#define _TRACKER_ON  //comment this out to disable tracker...

#ifdef _TRACKER_ON
#define GPS(aPtr) Tracker::instance().watch(aPtr, __LINE__, __FILE__)
#else
#define GPS(aPtr) (aPtr)
#endif

struct Tracker {

    static Tracker single;

    static Tracker& instance() {
        return single;
    }

    struct Memo {
        void*   ptr;
        size_t  line;
        size_t  filenum;
    };

    Tracker(bool aEnabled=false) {
        enabled=aEnabled;
        names.push_back("unknown");
    }

    bool isEnabled() const {return enabled;}

    Tracker& enable(bool aState) {
        enabled=aState;
        return *this;
    }

    Tracker& reset() { //called to forget prior ptrs...
        names.clear();
        list.clear();
        return *this;
    }

    void* track(void* aPtr) {
        if(enabled) {
            enabled=false;
            list.push_back(Memo{aPtr});
            enabled=true;
        }
        return aPtr;
    }

    template<typename T>
    T* watch(T* aPtr, size_t aLine=0, const char* aFile=nullptr) {
        if(aLine) {
            bool wasEnabled=enabled;
            enabled=false;
            std::string theName=fs::path(aFile).filename().u8string();
            auto theIt = find(names.begin(), names.end(), theName);
            size_t theIndex=names.size();
            if (theIt != names.end()) {
                theIndex = theIt - names.begin();
            }
            else names.push_back(theName);
            enabled=wasEnabled;

            auto theEnd=list.rend();
            for (auto it = list.rbegin(); it!=theEnd; it++) {
                if(it->ptr==aPtr) {
                    it->line=aLine;
                    it->filenum=theIndex;
                    break;
                }
            }
        }
        return aPtr;
    }

    Tracker& untrack(void* aPtr) {
        auto theIter = std::find_if(
                list.begin(), list.end(), [aPtr](Memo const& aMemo){
                    return aMemo.ptr == aPtr;
                });
        if(theIter != list.end()) {list.erase(theIter);}
        return *this;
    }

    Tracker& reportLeaks(std::ostream &aStream) {
        for(auto &theMem: list) {
            aStream << theMem.ptr << " : "
                    << names[theMem.filenum] << "("
                    << theMem.line << ")\n";
        }
        return *this;
    }

protected:

    Tracker(const Tracker &aTracker) {}

    bool                      enabled;
    std::vector<Memo>         list;
    std::vector<std::string>  names;
};

Tracker Tracker::single(false);

//-------------------------------------------

#ifdef _TRACKER_ON
void * operator new(size_t aSize) {
    auto thePtr=std::malloc(aSize);
    Tracker::instance().track(thePtr);
    return thePtr;
}

void * operator new[](size_t aSize) {
    return operator new(aSize);
}

void operator delete(void* aPtr) noexcept {
    Tracker::instance().untrack(aPtr);
    std::free(aPtr);
}

void operator delete[](void* aPtr) noexcept {
    operator delete(aPtr);
}
#endif

#endif /* Tracker_h */
