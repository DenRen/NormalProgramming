#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np
import sys

path=sys.argv[1]

data = np.loadtxt(path)

s = data[:, 0]
dt = data[:, 1]

plt.scatter(s, dt)
plt.grid()
plt.show()
