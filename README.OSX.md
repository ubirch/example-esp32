# Installation on OSX

## Install dependencies
```bash
brew tap ubirch/homebrew-esp32
brew install xtensa-esp32-elf
brew install esp-idf
```

## Setup Environment

You need to set the ESP PATH:

```
export IDF_PATH=`brew --prefix esp-idf`
```

### CLion (IntelliJ C/C++ IDE)
For use with [CLion](https://www.jetbrains.com/clion/) also add the config file: 
```
echo `brew --prefix esp-idf` > ~/.idf
```

Simply load the directory to initialize the build system.

### Visual Studio Code (Microsoft IDE)

- open [Visual Studio Code]() and load the project directory using `File -> Open`
- open the extensions window: `Code -> Preferences -> Extensions`
- Install the *CMake Tools*, *CMake language support* and *CMake Tools Helper*
- Visual Studio will also ask for the Compiler Kit:
    - Select `xtensa-esp32-elf`
- Before our project configures correctly, we need to add the `IDF_PATH` environment variable:
    - Open `Code -> Preferences -> Settings`
    - Select `Extensions` and `CMake Configuration`
    - Under `CMake Build Args` click on `Edit settings.json`
    - Add this fragment to the file in front of other settings and save:
        ```json
        "cmake.environment": {
            "IDF_PATH": "/usr/local/opt/esp-idf"
        },
        ```
    - Restart Visual Studio Code
    - After opening, if Code asks to configure the project, click `Yes`
- Compile all using `F7` or click at the bottom of the window on `Build`

## Compile and install 

Follow the explanation from [README](README.md).

> CLion: Select `app` or `app-flash` to build and flash, then click on build.

```
mkdir build
cd build
cmake ..
make app
make app-flash
```