Can use either Azure layer

cmake --build . --target clean
cmake ..
cmake --build . --target basic-sample

or build using PAHO

cmake --build . --target clean
cmake .. -DIOTC_USE_PAHO=ON
cmake --build . --target basic-sample
