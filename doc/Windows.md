
#### Building and Running with Visual Studio 2019
Build Tools for Visual Studio 2022
*** TBD *** https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
* Download and install [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/).
* Download and install [CMake](https://cmake.org/download/) with the installer. Ensure to add CMake to system path.
* Download and extract [Vcpkg](https://github.com/microsoft/vcpkg/releases).

clone the vcpkg Git repo into a directory with preferably a short global path from root of your drive,
bootstrap vcpkg and install the necessary dependencies. For example in command prompt:
```shell script
```

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

Run a new command prompt ensure. 


```shell script
cd <this repo clone directory>\samples\basic-sample
md build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg install directory>\scripts\buildsystems\vcpkg.cmake 
cmake --build . --target basic-sample 
.\Debug\basic-sample.exe
```

* If you wish to build with the Paho MQTT client, append ```-DIOTC_USE_PAHO=ON``` to the ```cmake ..``` command line.

#### Building and  Running with MSYS2 and CMake Command Line

The MSYS2 setup is less complex and faster, however the Azure IoT C SDK integration is not supported with MSYS2. 
You must use the Paho MQTT Client integration. 

* Download and Install MSYS2
* From MSYS2 bash shell, execute:

```shell script
pacman --sync --noconfirm base-devel gcc cmake openssl-devel libcurl-devel
cd <this repo sources>/samples/basic-sample
mkdir build
cd build
cmake .. -DIOTC_USE_PAHO=ON
cmake --build . --target basic-sample
./basic-sample.exe
```

#### Building and Running with CLion in Visual Studio Environment

* In CLion, open the *basic-sample* project from the *samples* directory of this repo
* In the top right of of the IDE next to the hammer icon, select *basic-sample*
* Select File->Settings->Build,Execution,Deployment->CMake and enter ```-DCMAKE_TOOLCHAIN_FILE=<vcpkg install directory>/scripts/buildsystems/vcpkg.cmake``` 
* If you wish to build with the Paho MQTT client instead of Azure SDK, Select File->Settings->Build,Execution,Deployment->CMake 
and appendd ```-DIOTC_USE_PAHO=ON``` in the "CMake options" entry box.
* Click the build or execute icon.


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
