## Instituto Tecnologico de Costa Rica
## Escuela de Ingenieria en Computacion
## Principios de Sistemas Operativos
### Prof.: Kevin Moraga

Luis Castillo
Janis Cervantes
Saul Zamora

### Proyecto 2
File system on user space

### Required dependencies
* GCC or Clang
* CMake >= 3
* make
* FUSE 2.6 or later
* FUSE development files

#### How to install dependencies
```
apt-get install gcc fuse libfuse-dev make cmake
```

### Build and run
1. Use CMake to check dependencies, setup environment and generate makefiles:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
```

2. Build project:
```
make -j
```
The `-j` parts tells make to parallelize the build to all your cores, remove it if you run out of CPU.

#### Run

1. Make a mount point, ie:
```
mkdir /tmp/example
```

2. Mount:
```
./bin/hrfs_lib -d -s -f /tmp/example
```

* `-d`: enable debugging
* `-s`: run single threaded
* `-f`: stay in foreground