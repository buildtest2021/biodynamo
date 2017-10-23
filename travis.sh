#!/bin/bash

set -e -x

echo ${TRAVIS_OS_NAME}
biod=`pwd`

# update and install packages
if [ "$TRAVIS_OS_NAME" = "osx" ]; then
  sw_vers
  osx_vers=`sw_vers -productVersion | cut -d . -f1 -f2`
  brew update >& /dev/null
  brew install doxygen
  brew install valgrind
  #brew install cloc
  # get clang 3.9
  brew install llvm
  export LLVMDIR="/usr/local/opt/llvm"
  export CC=$LLVMDIR/bin/clang
  export CXX=$LLVMDIR/bin/clang++
  export CXXFLAGS=-I$LLVMDIR/include
  export LDFLAGS=-L$LLVMDIR/lib
  export DYLD_LIBRARY_PATH=$LLVMDIR/lib:$DYLD_LIBRARY_PATH
  # get latest cmake
  brew install cmake
  export PATH=$LLVMDIR/bin:$PATH:
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test  # gcc-5
  wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
  sudo apt-add-repository -y "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main"
  sudo apt-get update
  sudo apt-get -y install openmpi-bin libopenmpi-dev
  sudo apt-get -y install freeglut3-dev
  sudo apt-get -y install gcc-5 g++-5
  sudo apt-get -y install valgrind
  sudo apt-get -y install doxygen
  sudo apt-get -y install cloc
  sudo apt-get -y install libiomp-dev
  sudo apt-get -y install clang-3.9 clang-format-3.9 clang-tidy-3.9
  export CC=gcc-5
  export CXX=g++-5

  # Need a CMake version >= 3.3 for VTK (Catalyst)
  cd
  wget https://cmake.org/files/v3.6/cmake-3.6.3-Linux-x86_64.tar.gz
  tar -xzf cmake-3.6.3-Linux-x86_64.tar.gz
  sudo rm /usr/bin/cmake
  sudo ln -s `pwd`/cmake-3.6.3-Linux-x86_64/bin/cmake /usr/bin/cmake

  # needed for Catalyst
  sudo ln -s /usr/lib/libmpi.so /usr/local/lib/libmpi.so
  sudo ln -s /usr/lib/libmpi.so /usr/local/lib/libmpi.so.12
fi

# install ROOT
cd
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  wget --progress=dot:giga -O root_v6.11.01_Linux-ubuntu14-x86_64-gcc5.4_263508429d.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=root_v6.11.01_Linux-ubuntu14-x86_64-gcc5.4_263508429d.tar.gz"
  tar zxf "root_v6.11.01_Linux-ubuntu14-x86_64-gcc5.4_263508429d.tar.gz" > /dev/null

  wget -O paraview-5.4_ubuntu14_gcc5.4_openmpi.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=paraview-5.4_ubuntu14_gcc5.4_openmpi.tar.gz"
  sudo mkdir -p /opt/biodynamo/paraview
  sudo tar -xzf paraview-5.4_ubuntu14_gcc5.4_openmpi.tar.gz -C /opt/biodynamo/paraview

  wget -O Qt5.9.1_ubuntu16_gcc5.4.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=Qt5.9.1_ubuntu16_gcc5.4.tar.gz"
  sudo mkdir -p /opt/biodynamo/qt
  sudo tar -xzf Qt5.9.1_ubuntu16_gcc5.4.tar.gz -C /opt/biodynamo/qt

  export ParaView_DIR=/opt/biodynamo/paraview/lib/cmake/paraview-5.4
  export Qt5_DIR=/opt/biodynamo/qt/lib/cmake/Qt5

  export LD_LIBRARY_PATH=/opt/biodynamo/qt/lib:/usr/lib/openmpi/lib:$LD_LIBRARY_PATH
else
  wget --progress=dot:giga -O root_v6.11.01_macos64_LLVM-Clang-5.0_263508429d.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=root_v6.11.01_macos64_LLVM-Clang-5.0_263508429d.tar.gz"
  tar zxf "root_v6.11.01_macos64_LLVM-Clang-5.0_263508429d.tar.gz" > /dev/null

  wget -O paraview-5.4_macos64_llvm-5.0.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=paraview-5.4_macos64_llvm-5.0.tar.gz"
  sudo mkdir -p /opt/biodynamo/paraview
  sudo tar -xzf paraview-5.4_macos64_llvm-5.0.tar.gz -C /opt/biodynamo/paraview

  wget -O Qt5.9.1_macos64_clang81.tar.gz "https://cernbox.cern.ch/index.php/s/BbFptgxo2K565IS/download?path=%2F&files=Qt5.9.1_macos64_clang81.tar.gz"
  sudo mkdir -p /opt/biodynamo/qt
  sudo tar -xzf Qt5.9.1_macos64_clang81.tar.gz -C /opt/biodynamo/qt

  export ParaView_DIR=/opt/biodynamo/paraview/lib/cmake/paraview-5.4
  export Qt5_DIR=/opt/biodynamo/qt/lib/cmake/Qt5

  export DYLD_LIBRARY_PATH=/opt/biodynamo/qt/lib:/usr/lib/openmpi/lib:/usr/local/opt/llvm/lib:$DYLD_LIBRARY_PATH
  export DYLD_FRAMEWORK_PATH=${Qt5_DIR}/../..
fi

# set the Python envars for Catalyst
export PYTHONPATH=$ParaView_DIR/../../paraview-5.4/site-packages
export PYTHONPATH=$PYTHONPATH:$ParaView_DIR/../../paraview-5.4/site-packages/vtk

cd root
. bin/thisroot.sh

# output compiler information
echo ${CXX}
${CXX} --version
${CXX} -v

cd $biod


cloc .
# add master branch
# https://github.com/travis-ci/travis-ci/issues/6069
git remote set-branches --add origin master

# build biodynamo and run tests
mkdir build
cd build
mkdir install
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install ..
make -j2
make check-submission

# run following commands only on Linux
if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  # build snap package
  sudo apt-get -y install snapd
  sudo snap install core

  make snap-package

  # test snap package
  #   revert image to initial conditions
  sudo apt-get -y remove openmpi-bin libopenmpi-dev
  sudo apt-get -y remove freeglut3-dev
  sudo apt-get -y remove gcc-5 g++-5
  sudo apt-get -y remove valgrind
  sudo apt-get -y remove doxygen
  sudo apt-get -y remove cloc
  sudo apt-get -y remove libiomp-dev
  sudo apt-get -y remove clang-3.9 clang-format-3.9 clang-tidy-3.9
  export CC=""
  export CXX=""
  sudo rm /usr/bin/cmake
  sudo rm -rf /opt/biodynamo

  printenv
  export PATH=/snap/bin:$PATH

  ../test/integration/test_snap_package.sh
  TEST_RET_VAL=$?
  if [ "0" = "${TEST_RET_VAL}" ]; then
    echo "Upload snap package"
    # TODO upload
  else
    exit ${TEST_RET_VAL}
  fi
else
  brew tap Biodynamo/biodynamo
  brew install biodynamo
fi
