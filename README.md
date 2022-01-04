- [About](#about)
- [Building](#building)
  - [Requirements](#requirements)
- [Usage](#usage)

## About

This is a set of utilities to find perceptual similar images in a small to medium data sets.
It's not usefull on data sets which contains more than approx. 1000000 images since method used
becomes not very accurate.

## Building

1. [Install vcpkg](https://vcpkg.io/en/getting-started.html).
2. `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux-release -DCMAKE_BUILD_TYPE=Release`
3. `cmake --build build`
4. Binaries will be located in the `build` directory.

Compilation was tested on Ubuntu 18.04+ with packages from the list below installed by stantard
means (apt-get install etc.).

### Requirements

* GCC >= 4.8 (for C++11 features)
* CMake
* [GraphicsMagick](http://www.graphicsmagick.org/)

All other dependencies will be managed by vcpkg & cmake.

## Usage

Example dataset can be downloaded from [here](https://s3-eu-west-1.amazonaws.com/sigterm.ru/public/imgdupl-dataset-example.tar)

* You need to build perceptual hashes for the images.
```
$ ./imghash --data /tmp/imgdupl-dataset-example --result /tmp/hashes.txt
```
* You need to export results into SQLite database.
```
$ ./export2db hashes /tmp/hashes.txt /tmp/imgdupl.db
```
* You need to clusterize images.
```
$ ./clusterizer /tmp/imgdupl.db 32 2 >/tmp/clusters_32.txt
```
* You need to export results of clusterization stage into SQLite database.
```
$ ./export2db clusters /tmp/clusters_32.txt /tmp/imgdupl.db clusters_32
```
* Print clusters.
```
$ print-clusters --database /tmp/imgdupl.db --table clusters_32 --min-size 3
```

You may vary perceptual hashes matching threshold constant (second argument to the clusterizer utility) to
improve quality.

If you want to view clusterization results more visually you can run viewer:

1. `python3 -m venv .venv`
2. `source .venv/bin/activate`
3. `pip install --upgrade pip`
4. `pip install -r viewer/requirements.txt`
5. `./viewer/webstand.py 127.0.0.1:9090 /tmp/imgdupl.db clusters_32`
