#### Initial Setup

Install cmake, make and an adequate C compiler and tools before building. This can be done on Ubuntu by executing:
```shell script
sudo apt-get install build-essential cmake 
``` 
On Ubuntu, you can run the following command to satisfy the library dependencies: 

```shell script
sudo apt-get install libcurl4-openssl-dev libssl-dev uuid-dev
```

#### Git Setup

If you cloned the repo, execute ```scripts/setup-project.sh```. 
The script will pull the dependency submodules and it will ensure 
that you don't accidentally check in your device credentials or account information in app_config.h.

#### Building and Running with CMake Command Line
  * Initialize the project using the script on Linux/MacOS with ```scripts/setup-project.sh```

```shell script
cd samples/basic-sample
mkdir build
cd build
cmake ..
cmake --build . --target basic-sample
./basic-sample
```

#### Building and Running with CLion

* In CLion, open the *basic-sample* CMakeLists project from the *samples* directory of this repo
* You may want to point the proejct root to the root of this repo.
* In the top right of of the IDE next to the hammer icon, select *basic-sample*
* Click the build, execute or debug icon.
