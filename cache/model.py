#!/bin/python3

import matplotlib.pyplot as plt
import numpy as np

def model_func(step, T, t, L, l):
    if step < l:
        return (T + l * t / step) * (L // l)
    else:
        return T * (L // step)

def model_func2(step, T, t, L, l):
    if step < l:
        N = (L // l) * l
        return (T + l * t / step) * (L // l) / N
    else:
        N = L // step
        return T * (L // step) / N

size = 1024
x = np.linspace(1, size, size)
y = np.zeros(size)
for i in range(0, size, 1):
    y[i] = model_func2(x[i], 100, 1, 64000, 64)

plt.plot(x, y)
plt.scatter(x, y)

plt.show()
