
#### Building and Running with Visual Studio 2019

* Download and install [Git for Windows](https://git-scm.com/download/win). Ensure to add git bash path when/if prompted. This can be useful in other projects.
* Download and install [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022).
* Download and install [CMake](https://cmake.org/download/) with the installer. Ensure to add CMake to system path.

Clone or download the vcpkg Git repo into a directory with preferably a short global path from root of your drive,
bootstrap vcpkg and install the necessary dependencies.
Run a command prompt and execute (use x86 instead of x64 if on a 32-bit machine):

```shell script
md c:\tools
cd C:\tools
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe integrate install
.\vcpkg\vcpkg.exe install curl:x64-windows
.\vcpkg\vcpkg.exe install openssl:x64-windows
exit
```

By exiting the command prompt we ensure that we pick up the "integrate install" environment. 

Run a new command prompt to build and execute the basic-sample (with error if not yet configured):

```shell script
cd <itoc-generic-c-sdk clone directory>\samples\basic-sample
md build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg install directory>\scripts\buildsystems\vcpkg.cmake 
cmake --build . --target basic-sample 
.\Debug\basic-sample.exe
```
#### Git Setup

If you wish to use the git clone instead of the source packages from this repo Releases page:

* Install git for Windows with default setup options, or use MSYS2 git (see above) with pacman in bash shell
* Run:

```shell script
git clone <this-repo>
cd iotc-generic-c-sdk
git submodule update --init --recursive
git update-index --assume-unchanged samples/basic-sample/config/app_config.h
```

The submodule command will pull the dependency submodules and the update-index command will ensure 
that you don't accidentally check in your device credentials or account information in app_config.h.
