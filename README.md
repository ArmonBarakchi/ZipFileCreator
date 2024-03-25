
## Introduction

This is an Archival storage system. 
An archive file is a single file that contains other files, much like a folder on your 
hard drive contains other files. If you've used .archive or .zip files before, you get the idea. 
The documents stored are not changed in any way. When the user extracts a 
stored document, it is restored as an exact copy of the original. A file may be chosen to either
be compressed or not compressed through the IDataProcessor interface, which offers a compression class.
In the future, other types of IDataProcessors can be added.


## Specifics

Below, I will explain each interfaceable function of the Archive class. There are other primitives in this class
which will not be explained in this README


### Creating an Archive Storage File
Before a user can work with an __archive__ file, it must be created. This action creates a new (empty) archive file in
a standard location, ready to be used by the `Archive` tool.  Archive files use the ".arc" file extension.
So an archive file called "test" will actually have the name "test.arc".

To create a new archive storage file, the user will call the static method, `Archive::create`, along with the desired
filename. If this file already exists, it will be automatically truncated (emptied of all content and length set to
zero); otherwise the new (empty) file is created. This `Archive` object will already
have access with the opened associated binary file stream. If the file cannot be created, the `Archive::create` method
will return a `nullptr`.

```
static ArchiveStatus<std::shared_ptr<Archive>> createArchive(const std::string &anArchiveName, uint32_t aBlockSize=1024);
```

The argument `anArchiveName` is saved to the tmp folder with a ".arc" extension. `aBlockSize` is used
to determine how big the data load is for each block in the Archive.

### Opening a Pre-existing Archive Storage File
Presuming that an existing __archive__ file already exists on disk, the user may choose to open that file for use.
To open an __archive__ file, a user will call the static method, `Archive::open`, and pass the name of the file to be
opened. Presuming that it's a legitimate __archive__ file, a new `Archive` object is returned, with the associated file
opened and ready for use.  If the file doesn't exist (or isn't a real archive file) -- the `Archive::open` method will
return a `nullptr`.

```
static ArchiveStatus<std::shared_ptr<Archive>> openArchive(const std::string &anArchiveName);
```

The argument `anArchiveName` must be the name of an Archive in the tmp directory used for this project

### Archive Tool Commands

#### __Add__ file to Archive
Assuming an open archive file (new or pre-existing), the user can add a new document to the archive by calling the
`Archive::add` method. The signature for this method is given below:

```
ArchiveStatus<bool> add(const std::string &aFilename, IDataProcessor* aProcessor=nullptr);
```

The `aFilename` will contain the complete path (path+name) of a file to be added to the stream.  For example:
"/tmp/small.txt".   The `Archive::add` method will copy the contents of the given file into a series of 1..n
blocks within the archive itself. The document will be associated in the archive with the filename portion of
the given full-path, so that it may be retrieved (extracted) or removed in the future. If everything goes works,
this function call will return `True`.  For example, if `aFilename` is "/tmp/small.txt", the name stored for
the document is "small.txt" (excludes the path portion).

The `IDataProcessor` is a pointer to a pre-process that can be ran on the file before being added. The specific processor
for each file will be remembered and the reverse process will be ran before extraction. 

#### __Extract__ file from Archive
Assuming an open archive file (new or pre-existing), the user can extract a named resource inside the archive file.
When the user calls this message and passes a document name, the archive will be searched for that file. If the
file exists in the archive, the contents of the file will be written from the storage blocks to a file at the
given `aFullOutputPath`, and return `True`. If the named resource does not exist, `Archive::extract()` will stop and return
`False`.


```
bool extract(std::string &aName, const std::string &aFullOutputPath);
```

#### __Remove__ file from Archive
Assuming an open archive file (new or pre-existing), the user can __remove__ a named resource inside the archive file.
When the user calls this message and passes a document name, the archive will be searched for that file. If the
file exists in the archive, the named file will be removed from the archive. Generally, this means that any internal
block occupied by the document will be marked as "available" for reuse in a subsequent write operation in the archive.
If this succeeds, `True` is returned. If the document name is not found in the archive, `False` is returned to the caller.

```
bool remove(std::string &aName);
```

#### __List__ files in Archive
Assuming an open archive file (new or pre-existing), the user can __list__ all the names of the documents stored inside
the archive file. When the user calls this message, all the documents stored internally will be located, and printed
 to the given `std::ostream` object. The number of files in the Archive is returned. Sample output format documents
in the archive is given below:

```
###  name         size       
---------------------------
1.   document1    203,400   
2.   document2    2,150     
```

Here is the __list__ method call:
```
size_t list(std::ostream &aStream);
```

#### __Dump__ archive blocks (debug mode)
Assuming an open archive file (new or pre-existing), the user can __dump__ information about the sequence of blocks
stored inside the archive file. When the user calls this message, the blocks are iteratd (sequentially from first to last),
and meta information is printed about the block to the given `std::ostream` object. When the function is finished, the number
of blocks in the archive is returned. Sample output of the dump is given below:

```
###  status   name    
-----------------------
1.   empty    
2.   used     test.txt
3.   used     test.txt
4.   empty
```

Here is the __dump__ method call:
```
size_t debugDump(std::ostream anOutputStream);
```


#### __Compact__ archive file
This method will result in the archive file removing empty blocks. 
Once complete, the archive file will contain 0 empty blocks, and every block will contain data from an archived document.
This method returns the total number of blocks in the given archive file.

```
ArchiveStatus<size_t> compact();
```


#### Merging Archives

```cpp
class Archive {
  //...
  ArchiveStatus<bool> merge(const std::string &anotherArchivePath);
};
```

This function will open the other archive, iterate all the files in the other archive, and copy each file into the current archive/
The other archive is not changed in the process. 

### Storing and Retrieving folders/directories

The archive file system supports adding and extracting folders, not just individual files 

#### Adding Folders


```cpp
class Archive {
  //...
  ArchiveStatus<bool> addFolder(const std::string &aFolder);
}
```

The argument, `aFolder`, will contain the path to the folder to be added.
Every file within this folder is added to the Archive. In addition, the folder to which
each file is associated with is saved.  Note: the folder cannot contain subfolders


Iteration of files in folder is done using this ScanFolder Class
```cpp
ECE141::ScanFolder theScan(aFolder);
theScan.each([](const fs::directory_entry &anEntry) {
  if(anEntry.is_regular_file()) 
    std::cout << "This is a file: " << anEntry.path() << "\n";
  else
    std::cout << "This is a folder: " << anEntry.path() << "\n";
  
  return true;
});
```

The `ScanFolder` class provides an `each(...)` method which takes in a lambda function.
This lambda function will be called for every item (whether that be a file or a folder) in
the folder. This lambda function should also return a `bool`, where `true` tells the
`ScanFolder` to keep iterating, and `false` tells it to stop.



#### Extracting Folders


```cpp
class Archive {
  //...
  ArchiveStatus<bool> extractFolder(const std::string &aFolderName, const std::string &anExtractPath);
}
```

Arguments:
1. `aFolderName`: The name of the folder which contained the files we want to extract.
2. `anExtractPath`: This specifies a folder where the extracted files should go.

Example Usage: If we added the `"sub"` folder to our Archive, we can call
`extractFolder("sub", "output_folder/");`. This will take all the files previously added
to the archive (from the `"sub"` folder) and extract them into the `"output_folder/"` folder.


### Miscellaneous

#### Data Processing Interface
The IDataProcessor interface represents a fundamental architectural enhancement of the archival system. It provides a blueprint for data processing capabilities, enabling the system to execute sophisticated operations on data both before archival and upon retrieval. Implementations of this interface may include a variety of data transformations, ensuring adaptability and extensibility to meet future requirements.

- Process: A method designed for processing data prior to archival. This includes operations such as validation, encoding, or preparation for compression, facilitating a versatile preparation phase.
- ReverseProcess: A method aimed at reverting processed data back to its original state during extraction. This crucial feature ensures that the integrity of the data is preserved, allowing documents to be restored to their exact pre-archival state.

I have implemented a simple compression processor as an example for how this IDataProcessor base class can be used 
to expand the functionality of the Archive class.

#### Status Management with ArchiveStatus
The ArchiveStatus template class is instrumental in managing the operational status of actions performed within the archival system. It offers a unified approach to handling both successful outcomes and errors, providing clear and concise feedback mechanisms for the system's operations.

- Success Case: For operations like adding, extracting, or processing documents that are completed successfully, ArchiveStatus encapsulates the result. This encapsulation allows the system to confidently proceed with subsequent tasks.

- Error Case: In instances where operations face issues, ArchiveStatus identifies and captures the specific error encountered. This enables appropriate responses, such as error logging, user notification, or corrective actions, thus ensuring the system’s stability and reliability.

#### Robust Error Handling
To enhance the system's robustness, an enum class of potential errors (ArchiveErrors) has been defined. This enum class addresses a broad range of potential system issues,
from file access errors to data integrity concerns. Through explicit error handling, the system's resilience and reliability are significantly improved,
ensuring that users are well-informed about the state of the system and any encountered issues. Proper implementation of this error handling mechanism is under work.

### A Note About Storage Files
The storage system will read/write archive files as sequence of fixed size blocks. Archive
files will be opened in binary mode using C++ filestreams.

Each block in the storage system will be exactly 1024 (1K) in size. Ideally, the amount of meta data per block should be limited.
Although this is not the most efficient solution for meta data usage, it simplifies the implementation to be more digestible.

```
[BLOCK
  [ meta area - 51 bytes]
  [ data payload -- 973 bytes..............................................]
]  
```

