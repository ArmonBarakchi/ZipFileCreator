[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-24ddc0f5d75046c5622901739e7c5dd533143b0c8e959d652212380cedb1ea36.svg)](https://classroom.github.com/a/aTyhvTsR)
# WI24-Final

Further expanding the archive project.

### Due Mar 21, 2023 at 6:00pm (PST)

## Introduction

Welcome to your ECE 141A Final! Don't stress (at least not too much) as this will be a fun one!
This should feel like a PA except shorter -- you can even consider this to be part 3 of the Archive
project.

This final is closed notes, but you are allowed to use https://devdocs.io/cpp/.

Please, grab your existing Archive code and use it for this final. Be sure **NOT** to replace the new
`Testing.hpp`, `main.cpp`, and `CMakeLists.txt`, as we have added new functionality to these files.

To make sure your code correctly compiles, be sure to do the following:

1. Update the new `CMakeLists.txt` with any additional C++ header files that you may have added.
2. Update the `getLocalFolder()` function in `main.cpp`.
3. If you're a Windows user that had issues with Zlib, don't worry, it is not used in this final.

### Overview
For this final, we will be asking you to expand your Archive project in 3 ways:
1. Increase the block/chunk size from 1024 to 2048 bytes.
2. Create a method to merge multiple archives into a singular archive.
3. Allow the archive to not just store files, but also folders/directories.

We have tried to order these in increasing difficulty, but feel free to do these in any order!

## Specifics

Below, we have created 3 challenges based on your existing Archive codebase. If possible, implement each of the following challenges. If you can't fully implement one, a good design strategy is still worth considerable points.  Document the design you would create to solve the challenge, and save it in a file called "design.txt".

## Challenge #1 - Increasing the block/chunk size.

NOTE TO EMIN: Perhaps we should limit the sizes to 1024 and 2048, or maybe 3072?

In this challenge, your task is to modify the create archive method to use a variable block size. Up until now you have
been using the default block size of 1024 bytes. Now the block size will be a parameter to the `create` method. You should
also update the `Archive` class to store the block size as a member variable and persist it to the archive file so that
it can be read back in when the archive is opened.

Here is how the updated `create` method should look:
```cpp
class Archive {
  ...
  ArchiveStatus<bool> create(const std::string &aPath, uint32_t aBlockSize);
};
```

The test for this will create an archive with block size 2048 add our usual set of files to it and then dump the contents
to make sure correct number of blocks are outputted.


## Challenge #2 - Merging Archives
In this challenge, your task is to merge the contents of one archive into another. You will add a new method to your `Archive` class:

```cpp
class Archive {
  //...
  ArchiveStatus<bool> merge(const std::string &anotherArchivePath);
};
```

When called, you will open the other archive (from given path). Next, iterate all the files in the other archive, and copy each file into the current archive. The "other" archive will not be changed in this process.  Make sure to save all the changes to your current archive.

NOTE: Don't overthink this challenge!  You probably have most of the code you need to do this already. Think about how streams might make this task simpler.

## Challenge #3 - Storing and Retrieving folders/directories

For this part of the assignment, you will be updating your Archive class to support the adding
of entire folders (not just individual files)! There are two halves to this part:

1. Implementing the `addFolder(...)` method.
2. Implementing the `extractFolder(...)` method.

### Adding Folders

Go ahead and add this new method to your Archive class:

```cpp
class Archive {
  //...
  ArchiveStatus<bool> addFolder(const std::string &aFolder);
}
```

The argument, `aFolder`, will contain the path to the folder we want you to add.
Your job is to add every file within this folder to your Archive. In addition, you
will need to store which folder each of these files belong to.  Note: the folder you're asked to store only contains files, and no other sub-folders. This can simplify your work.

To help you complete this task, we provide a handy `ScanFolder` utility class,
which you can use to iterate through all the files in a folder.
Here is an example of how it can be used:

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

So for example, imagine we have the following files:
```
tmp/sub/largeA.txt
tmp/sub/smallA.txt
```

If `addFolder("tmp/sub/")` gets called, then our Archive should add both `largeA.txt`
and `smallA.txt`. In addition, the Archive should remember that these two files were within
the `"sub"` folder. You will see why this is important in the `extractFolder()` method.

So when I run the Archive's `list()` method, I should see the following:

```
##      name             size       date added
--------------------------------------------------
1       sub/largeA.txt   2678       2024-03-15 10:15:00
2       sub/smallB.txt   906        2024-03-15 10:15:00
```

### Extracting Folders

Now that we can add folders, let's now add the ability to extract them.

Go ahead and add this new method to your Archive class:

```cpp
class Archive {
  //...
  ArchiveStatus<bool> extractFolder(const std::string &aFolderName, const std::string &anExtractPath);
}
```

This method has two arguments:
1. `aFolderName`: The name of the folder which contained the files we want to extract.
2. `anExtractPath`: This specifies a folder where the extracted files should go.

Going off the previous example where we added the `"sub"` folder to our Archive, we can call
`extractFolder("sub", "output_folder/");`. This will take all the files we previously added
to our archive (from the `"sub"` folder) and extract them into the `"output_folder/"` folder.

So after this method has been executed, we should see the following files:
```
output_folder/largeA.txt
output_folder/smallA.txt
```

## Testing
Just like in all the PA's, we provide automated tests to grade and validate your solutions.

### Grading Rubric
Your solution will be graded using the following rubric:

1. Compile test -- 10%
2. Larger block size test -- 30%
3. Combining archives test -- 30%
4. Add folder test -- 30%

## Submitting your work

Due Mar 17, 2023 at 6:00pm (PST).

Make sure to turn in your code, and update the associated `student.json` file.
