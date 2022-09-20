#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np

path="res_native"

data = np.loadtxt(path)

col_row_size = data[:, 0]
time = data[:, 1]

x = np.linspace(col_row_size.min(), col_row_size.max())
poly = np.polyfit(col_row_size, time, 3)
y = np.polyval(poly, x)

plt.plot(x, y, color='r', linestyle = '--')

plt.scatter(col_row_size, time, color='blue', marker='x')
plt.xlabel("Size")
plt.ylabel("Time")
plt.show()
