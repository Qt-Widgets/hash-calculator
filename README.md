# Hash Calculator

A simple tool to compute hash value for files.

This application's icon was designed, created and edited by [Lei Zhang](https://github.com/GA-1101). Thanks very much!

**IMPORTANT NOTE**

Although this tool supports open folders, you should not open folders which contain too many files, otherwise the application may be busy searching for files and the UI will stuck there. I will not optimize this because it is not designed to do such things. This *small* tool is designed be used to calculate hash value for *only a small amount* of files.

## Screenshots

![Main window](/screenshot.png)

## Compilation

- QMake:

  ```text
  qmake
  jom/nmake/mingw32-make/make
  ```

- CMake:

  ```text
  cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<your-qt-dir> -GNinja <hc-source-code-dir>
  cmake --build .
  ```

Note: In source build is not recommended. Please create a separate directory to contain the meta files generated by QMake/CMake before configuring and compiling.
