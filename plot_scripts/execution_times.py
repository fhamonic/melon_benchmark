import matplotlib.pyplot as plt
import csv
import numpy as np
import os
import sys

dir = sys.argv[1]
dataset_name = os.path.split(dir)[1]
algo_name = os.path.split(os.path.split(dir)[0])[1]
csv_files = list(filter(lambda f: f.endswith('.csv'), os.listdir(dir)))
csv_files.sort()
csv_paths = [dir + "/" + csv_file for csv_file in csv_files]


def readCSV(file_name, delimiter=','):
    file = csv.DictReader(open(file_name), delimiter=delimiter)
    return list([row for row in file])


libs = [os.path.splitext(f)[0] for f in csv_files]
instances = [row['instance'] for row in readCSV(csv_paths[0])]
data = [
    [int(float(row['time_ms']))
     for row in readCSV(csv_file)] for csv_file in csv_paths
]
x = np.arange(len(instances))

width = 0.225  # width of the bars
fig_size = plt.rcParams["figure.figsize"]
fig_size[0] = 10
fig_size[1] = 6
plt.rcParams["figure.figsize"] = fig_size
plt.rcParams.update({'font.size': 10})

fig, ax = plt.subplots()

rect_plots = [ax.bar(x - (len(libs) - 1) * width/2 + i * width,
                     data[i], width, label=libs[i]) for i in range(len(libs))]


ax.set_ylabel('miliseconds')
ax.set_title(
    '{} average runtime on {} dataset'.format(algo_name, dataset_name))
ax.set_xticks(x)
ax.set_xticklabels(instances)
plt.xticks(rotation=60)
ax.legend()


def autolabel(rect_plot):
    for rect in rect_plot:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


for rect_plot in rect_plots:
    autolabel(rect_plot)

fig.tight_layout()

plt.savefig('{}/../../{}_{}.png'.format(dir, algo_name, dataset_name))
