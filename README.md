# Code Lybyrinth (LLVM Obfuscation Pass)


## Features

- [Fake Instruction Inserter](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Fake-Instruction-Inserter)
- [Dynamic Call Converter](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Dynamic-Call-Converter)
- [Branch Address Encryptor](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Branch-Address-Encryptor)
- [String Encryptor](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-String-Encryptor)

## Build for iOS  (on Mac)

### Compile Clang
[How to build clang and swift‐frontend (on Mac M2)](https://github.com/codetronik/CodeLabyrinth/wiki/How-to-build-clang-and-swift%E2%80%90frontend-(on-Mac-M2))

### Compile Pass Plugin
```sh
$ git clone https://github.com/codetronik/CodeLabyrinth
$ cd CodeLabyrinth
$ cmake -B Release -DLLVM_DIR=/SWIFT_CLONE_PATH/build/buildbot_osx/llvm-macosx-arm64/lib/cmake -DCMAKE_OSX_ARCHITECTURES=arm64
$ cd Release
$ make -j16
```

### Xcode's Clang option
Project Setting -> Apple Clang -> Custom Compiler Flags -> Other C Flags -> Release  
> -fpass-plugin=/libCodeLabyrinth.dylib -DCMAKE_OSX_ARCHITECTURES=arm64

## Build for Android (on Windows)

### Compile Clang
[How to compile LLVM Pass for Android using Visual Studio](https://github.com/codetronik/CodeLabyrinth/wiki/How-to-compile-LLVM-Pass-for-Android-using-Visual-Studio)

### Compile Pass Plugin
```sh
$ git clone https://github.com/codetronik/CodeLabyrinth
$ notepad.exe CodeLabyrinth\windows\Directory.build.props
```
Modify ```LLVMINstallDir``` then open ```CodeLabyrinth.sln``` and build.

### Visual Studio's Clang option
Project Setting -> Configuration Properties -> C/C++ -> Command Line -> Additional Options
> -fpass-plugin=\CodeLabyrinth.dll

## Test Environment
- iOS : C++, Objective C++ / Xcode 15.0.1 / iPhone 12 mini
- Android : C++ / Visual Studio 2022 / Galaxy Note S20
- Windows : C++ with Command Line
