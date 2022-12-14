# Matrix
В этом проекте реализованы 4 версии перемножения матриц:
* Наивное mxnv::Matrix
* Наивное с транспонирование правой матрицы mxtr::Matrix
* Блочно-транспонированное mxcl::Matrix
* Блочно-транспонированное в многопоточном режиме

Оказалось, что кэш-эффекты играют значительную роль в высокопроизводительных вычеслениях. Реализация mxcl быстрее наивной в 24 раза на размере матрицы 18 МБайт.

## Need to install
```bash
sudo apt update
sudo apt install -y build-essentia gcc make cmake
```

## Build and run
```bash
mkdir build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./run_test_matrix
./matrix
```
## How to generate data for graphics
Build, use instruction above
```bash
cd build
./graphics
cp time_* ../results
```

## How to plot graphics
```bash
cd scripts
./plot.py

```