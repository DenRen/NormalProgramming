#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np

path_nv="../results/time_native"
path_cl="../results/time_cachelike"
path_nvpl="../results/time_native_parallel"
path_clpl="../results/time_cachelike_parallel"

def ProcessTimeSize(path):
    data = np.loadtxt(path)

    size = data[:, 0]
    time = data[:, 1]

    # Approxymation of x^3
    app_x = np.linspace(size.min(), size.max())
    poly = np.polyfit(size, time, 3)
    app_y = np.polyval(poly, app_x)

    return [size, time, app_x, app_y]

def ProcessTimeNumThreads(path):
    data = np.loadtxt(path)

    num_threads = data[:, 0]
    time = data[:, 1]

    return [num_threads, time]

################################################################
# TimeSize -> Native
nv_size, nv_time, nv_app_x, nv_app_y = ProcessTimeSize(path_nv)

plt.subplot (2, 3, 1)
plt.plot(nv_app_x, nv_app_y, color='r', linestyle = '--')
plt.scatter(nv_size, nv_time, color='blue', marker='x')

plt.grid()
plt.xlabel("Size")
plt.ylabel("Time")
plt.title("Native implementation")

# TimeSize -> CacheLike
cl_size, cl_time, cl_app_x, cl_app_y = ProcessTimeSize(path_cl)

plt.subplot (2, 3, 2)
plt.plot(cl_app_x, cl_app_y, color='r', linestyle = '--')
plt.scatter(cl_size, cl_time, color='blue', marker='x')

plt.grid()
plt.xlabel("Size")
plt.ylabel("Time")
plt.title("Cache-like implementation")

################################################################
# TimeNumThreads -> Native
nvpl_num_threads, nvpl_time = ProcessTimeNumThreads(path_nvpl)

plt.subplot (2, 3, 4)
plt.scatter(nvpl_num_threads, nvpl_time, color='blue', marker='x')
plt.plot(nvpl_num_threads, nvpl_time, color='r', linestyle = '--')

plt.grid()
plt.xlabel("Number threads")
plt.ylabel("Time")
plt.title("Native implementation")

# TimeNumThreads -> CacheLike
clpl_num_threads, clpl_time = ProcessTimeNumThreads(path_clpl)

plt.subplot (2, 3, 5)
plt.scatter(clpl_num_threads, clpl_time, color='blue', marker='x')
plt.plot(clpl_num_threads, clpl_time, color='r', linestyle = '--')

plt.grid()
plt.xlabel("Number threads")
plt.ylabel("Time")
plt.title("Cache-like implementation")

################################################################
# TimeSize -> Native and CacheLike
plt.subplot (2, 3, 3)
plt.plot(np.log(nv_app_x), np.log(nv_app_y), color='r', linestyle = '--')
plt.plot(np.log(cl_app_x), np.log(cl_app_y), color='r', linestyle = '--')
plt.scatter(np.log(nv_size), np.log(nv_time), color='blue', marker='x')
plt.scatter(np.log(cl_size), np.log(cl_time), color='blue', marker='x')

plt.grid()
plt.xlabel("log(Size)")
plt.ylabel("log(Time)")
plt.title("Native and Cache-like implementation")

# TimeNumThreads -> Native and CacheLike
plt.subplot (2, 3, 6)
plt.scatter(nvpl_num_threads, np.log(nvpl_time), color='blue', marker='x')
plt.scatter(clpl_num_threads, np.log(clpl_time), color='blue', marker='x')
plt.plot(nvpl_num_threads, np.log(nvpl_time), color='r', linestyle = '--')
plt.plot(clpl_num_threads, np.log(clpl_time), color='r', linestyle = '--')

plt.grid()
plt.xlabel("Number threads")
plt.ylabel("log(Time)")
plt.title("Native and Cache-like implementation")

plt.show()