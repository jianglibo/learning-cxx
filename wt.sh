#!/bin/bash


# function to run ctest in the build folder.
function run_ctest {
    # Run ctest in the build directory
    cd build
    ctest  -V
    cd ..
}

# Watch for changes in CMakeLists.txt or source files
# exclude the enternal folder
# while inotifywait -e modify,move,create -r --exclude 'external' --exclude 'build' CMakeLists.txt .; do
while inotifywait -e modify,move,create -r --include 'lib' --include 'gtest' CMakeLists.txt .; do
    # Clear build directory (optional)
    # rm -rf build
    # Reconfigure and rebuild the project with CMake
    # cmake -S . -B build
    # rm -f build/Mxtest
    # rm -f build/T1Test
    # cmake --build build --parallel 14

    $CMAKE_HOME/bin/cmake --build /home/jianglibo/learning-cxx/build --config Debug --target all --
    # if [[ -f /home/jianglibo/bin/cmake ]]; then
        # /home/jianglibo/bin/cmake --build /home/jianglibo/learning-cxx/build --config Debug --target all --
    # else
        # /opt/cmake/bin/cmake --build /home/jianglibo/learning-cxx/build --config Debug --target all --
    # fi
    # /opt/cmake/bin/cmake --build /home/jianglibo/learning-cxx/build --config Debug --target all --
    # /opt/cmake/bin/cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++ -S/home/jianglibo/openpiv-c--qt -B/home/jianglibo/openpiv-c--qt/build -G Ninja
    # cmake --build . --parallel 14
    # run_ctest
done