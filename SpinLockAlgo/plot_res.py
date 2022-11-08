import matplotlib.pyplot as plt
import numpy as np

def plot_res(file_name, color, marker, marker_size):
    with open(file_name, 'r') as file:
        num_res = int(file.readline())

        lines = file.readlines()
        for line in lines:
            nums = np.fromstring(line, dtype=int, sep=' ')

            num_threads = np.ones(nums.size - 1) * nums[0]
            dtimes = nums[1:]

            # Plot scatter
            scatter = plt.scatter(num_threads, dtimes, c=color, s=marker_size, marker=marker)
        
        scatter.set_label(file_name)
        plt.legend()

plt.figure(figsize=[16,9])
plt.grid()
plt.xlabel('Number threads')
plt.ylabel('Common time (microseconds)')
plt.title('Time of work the spin-lock algorithms')

marker_size = 20
plot_res('res_TAS', 'blue', 'o', marker_size)
plot_res('res_TTAS', 'red', 'x', marker_size)
plot_res('res_TICKET_LOCK', 'green', '.', marker_size)
plot_res('res_RB_LOCK', 'Magenta', 'x', marker_size)

plt.savefig('res.png')
plt.show()