# NOMAD
NOMAD is a 12-DOF quadruped walking robot using Linear and Nonlinear Optimization Control techniques.

  - Linear and Nonlinear Model Predictive Control
  - Simultaneous Localization and Mapping
  - Remote Control Teleoperation

# Installation

The following instructions are for setup of a development environment for building executable code for Ubuntu 18.04.

Install Python Dependencies:
```
sudo apt-get install cython
pip install cython
```

Install [qpOASES 3.2.1](https://projects.coin-or.org/qpOASES/wiki/QpoasesDownload):
```
unzip qpOASES-3.2.1.zip
cd qpOASES-3.2.1
```
There seems to be a compilation/linking bug in 3.2.1. To fix it search for and  delete "-DUSE_LONG_INTEGERS" in make_linux.mk under CPPFLAGS.
```
make
sudo cp bin/libqpOASES.so /usr/local/lib
sudo cp bin/libqpOASES.a /usr/local/lib

sudo cp -r include/* /usr/local/include
ls

```

Install [Eigen 3.3.7](http://eigen.tuxfamily.org/index.php?title=Main_Page):
```
tar xvzf eigen-eigen-*.tar.gz
cd eigen-eigen-*
sudo cp -r Eigen /usr/local/include
```
 We also use some unsupported features of Eigen as well:
 ```
 sudo cp -r unsupported /usr/local/include
 ```
 
Install [bullet3](https://github.com/bulletphysics/bullet3):
 ```
git clone https://github.com/bulletphysics/bullet3
sudo apt-get install freeglut3-dev
cd bullet3/build3
cmake ..
make 
 ```

Install [DART](http://dartsim.github.io/install_dart_on_ubuntu.html):

If you will be uisng the Python binding you will need pybind:

```
git clone https://github.com/pybind/pybind11 -b 'v2.2.4' --single-branch --depth 1
cd pybind11
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DPYBIND11_TEST=OFF
make -j4
sudo make install
```

```
git clone git://github.com/dartsim/dart.git
cd dart
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release[Debug]
make -j4
```

You will also need to setup a data path for loading dart resourcees:

```
export DART_DATA_PATH=/usr/local/share/doc/dart/data/
```

If you would like to see some examples and tutorials make sure to build them as well:

```
make -j4 examples
make -j4 tutorials
```

Now install it:

```
sudo make install
```

Install [RBDL](http://dartsim.github.io/install_dart_on_ubuntu.html):

If you will be using the Python you will need this to setup the include directories:

```
sudo apt-get install python-numpy
```

```
wget https://bitbucket.org/rbdl/rbdl/get/default.zip
unzip default.zip
cd rbdl-rbdl-*
mkdir build
cd build/
cmake -D CMAKE_BUILD_TYPE=Release -D RBDL_BUILD_ADDON_URDFREADER=ON -D RBDL_BUILD_PYTHON_WRAPPER=ON ../
make -j4
```

Now install it:
```
sudo make install
```

RBDL should also support Python 3.  You will need to modify the following:
rbdl-rbdl-xxxxxxxxxx/python/CMakeLists.txt:

Change `SET (Python_ADDITIONAL_VERSIONS 2.7)`
to `SET (Python_ADDITIONAL_VERSIONS 3.*)`

Change `INSTALL ( TARGETS rbdl-python LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python2.7/site-packages/ )`
to `INSTALL ( TARGETS rbdl-python LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python3.*/dist-packages/ )`


## Now that the dependencies are installed let's install Nomad
Clone or Fork Nomad Repository:
```
git clone https://github.com/implementedrobotics/Nomad
```

Setup CMake Build Sources
```
cd Nomad/Software
mkdir Build
cd Build/
cmake ..
```

### Todos

 - Write MORE Tests

License
----

MIT


