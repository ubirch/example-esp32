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

> For use with CLion also add the config file: 
>	```
>	echo `brew --prefix esp-idf` > ~/.idf
>	```

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