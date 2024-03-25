//
//  ScanFolder.hpp
//
//  Created on 2/13/24.
//

#ifndef ScanFolder_h
#define ScanFolder_h

#include <iostream>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace ZipFileCreator {

/*
  fs::directory_entry methods:
    - std::fs::path path() const;
    - bool          is_regular_file() const;
    - bool          is_directory() const;
    - uintmax_t     file_size() const;
*/

/* --------- Example use: -------------

ECE141::ScanFolder theScan("/tmp");
theScan.each([](const fs::directory_entry &anEntry) {
  if(anEntry.is_regular_file()) {
    std::cout << anEntry.path() << "\n";
  }
  return true;
});

*/

  using ScanObserver = std::function<bool(const fs::directory_entry&)>;

  class ScanFolder {
    fs::path path;
  public:
    ScanFolder(const std::string& aPath) : path(aPath) {}

    bool each(ScanObserver anObserver) {
      bool theResult{true};
      if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto &theEntry : fs::directory_iterator(path)) {
          theResult=anObserver(theEntry);
          if(!theResult) break;
        }
      }
      return theResult;
    }
  };

}





#endif /* ScanFolder_h */
