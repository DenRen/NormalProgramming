#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np

path="result/result"

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
size, time, app_x, app_y = ProcessTimeSize(path)

plt.plot(app_x, app_y, color='r', linestyle = '--')
plt.scatter(size, time, color='blue', marker='x')

plt.grid()
plt.xlabel("Number threads")
plt.ylabel("Time")
plt.title("Split-ordered list")

plt.show()
# plt.savefig(path + '.jpg')