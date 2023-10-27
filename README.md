# Code Lybyrinth (LLVM Obfuscation Pass)


## Features

- [Fake Instruction Inserter](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Fake-Instruction-Inserter)
- [Dynamic Call Converter](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Dynamic-Call-Converter)
- [Branch Address Encryptor](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-Branch-Address-Encryptor)
- [String Encryptor](https://github.com/codetronik/CodeLabyrinth/wiki/%5BPASS%5D-String-Encryptor)

## Build for iOS  (on Mac)

### Compile Clang
Download the same version as clang of your Xcode. Check your version [here](https://en.wikipedia.org/wiki/Xcode)<br>
Xcode 15.0.1 uses clang-1500.0.40.1 and is included in swift-5.9 branch.

```sh
$ git clone -b swift-5.9-RELEASE --depth 1 --single-branch https://github.com/apple/llvm-project.git
$ cd llvm-project
$ cmake -S llvm -B Release -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_NEW_PASS_MANAGER=ON -DCMAKE_OSX_ARCHITECTURES=arm64 -DLLVM_ENABLE_PROJECTS="clang" 
$ cd Release
$ make -j16
```

### Compile Pass Plugin
```sh
$ git clone https://github.com/codetronik/CodeLabyrinth
$ cd CodeLabyrinth
$ cmake -B Release -DLLVM_DIR=/YOURPATH/llvm-project/Release/lib/cmake -DCMAKE_OSX_ARCHITECTURES=arm64
$ cd Release
$ make -j16
```

### Xcode's Clang option
Project Setting -> Apple Clang -> Custom Compiler Flags -> Other C Flags -> Release  
> -fno-legacy-pass-manager -fpass-plugin=/libCodeLabyrinth.dylib -DCMAKE_OSX_ARCHITECTURES=arm64

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
