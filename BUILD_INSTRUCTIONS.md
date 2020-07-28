# How to build this project

## Windows

## macOS

- CMake version 3.17.3 or newer. Download the binary distribution from [here](https://cmake.org/download/) or execute this command:

```sh
$ brew install cmake
```

- GLFW version 3.3 or newer. Download the binary distribution from [here](https://www.glfw.org/download.html) or execute this command:

```sh
$ brew install glfw
```

- Qt. I use version 5.15.0, but older ones should work too.
 - Download the Qt Online Installer from [here](https://www.qt.io/download-qt-installer) and install Qt for open source development.
 - If you don't have Xcode installed, you will see a warning that says "You need to install Xcode and set up Xcode command line tools...". You can ignore it.
 - In the "Select Components" page, select the "macOS" option that's under "5.15.0" and don't change anything else.
 - Alternatively, you can also install Qt using the command below, although I have never done it this way so I don't know which components it installs.

```sh
$ brew install qt
```

- Launch CMake
- In the "Where is the source code:" field, enter the path to the root of this repository, that is, the directory that contains CMakeLists.txt
- In the "Where to build the binaries:" field, enter the path to the directory where you want the project to be built. It's best if this directory is outside of this repository.
- Click on the "Configure" button. You will see an error like this:
- To fix it, enter the path to the following directory of your Qt installation in the "Qt5_DIR-NOTFOUND" field: /Qt/5.15.0/clang_64/cmake/qt5. In my case, the full path is /Users/diegomacario/Qt/5.15.0/clang_64/cmake/qt5, but this might change depending on where you installed Qt.
- Click on the "Configure" button again.
- The configuration should succeed.
- Click on the "Generate" button.
- The generation should succeed.
- You should now be able to build the project by executing the following command in the directory that you entered for the "Where to build the binaries:" field:

```sh
$ make
```

- The build should succeed.
- Finally, you should be able to launch this project by executing the following command in the same directory:

```sh
$ ./Dyna-Kinematics
```

## Linux
