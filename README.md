This is a set of utilities to find perceptual similar images in a small to medium data sets.
It's not usefull on data sets which contains more than approx. 1000000 images since method used
becomes not very accurate.

## Building

This is a standard CMake project.

Compilation was tested on Ubuntu 14.04 with packages from the list below installed by stantard
means (apt-get install etc.).

### Requirements:
* GCC >= 4.8 (for C++11 features)
* CMake
* [GraphicsMagick](http://www.graphicsmagick.org/)
* Boost
* [Eigen3](http://eigen.tuxfamily.org/)
* Sqlite3

## Usage

Example dataset can be downloaded from [here](https://s3-eu-west-1.amazonaws.com/sigterm.ru/public/imgdupl-dataset-example.tar)

* You need to build perceptual hashes for the images.
```
$ ./imghash /tmp/imgdupl-dataset-example >/tmp/hashes.txt
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

If you want to view clusterization results more visually you can run www stand from the viewer directory:
```
$ ./webstand.py 127.0.0.1:9090 /tmp/imgdupl.db clusters_32
```
