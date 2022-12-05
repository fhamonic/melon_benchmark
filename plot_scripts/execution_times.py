import matplotlib.pyplot as plt
import csv
import numpy as np
import os
import sys

print(sys.argv[2])

target_name = sys.argv[1]
csv_paths = sys.argv[2].split(' ')

output_file_name = '_'.join(target_name.split('-')[1:])
output_file_path = '/'.join(csv_paths[0].split('/')
                            [:-3]) + '/' + output_file_name + ".png"
algo_name = target_name.split('-')[1]
dataset_name = target_name.split('-')[2]
cpu_name = csv_paths[0].split('/')[1]


def readCSV(file_name, delimiter=','):
    file = csv.DictReader(open(file_name), delimiter=delimiter)
    return list([row for row in file])


legend = [csv_path.split('/')[-1].split('.')[0] for csv_path in csv_paths]
instances = [row['instance'] for row in readCSV(csv_paths[0])]
data = [
    [int(float(row['time_ms']))
     for row in readCSV(csv_file)] for csv_file in csv_paths
]
x = np.arange(len(instances))

width = 0.9 / len(legend)
fig_size = plt.rcParams["figure.figsize"]
fig_size[0] = 10
fig_size[1] = 6
plt.rcParams["figure.figsize"] = fig_size
plt.rcParams.update({'font.size': 10})

fig, ax = plt.subplots()

rect_plots = [ax.bar(x - (len(legend) - 1) * width/2 + i * width,
                     data[i], width, label=legend[i]) for i in range(len(legend))]

ax.set_ylabel('miliseconds')
ax.set_title(
    '{} average runtime on {} dataset\nwith {}'.format(algo_name, dataset_name, cpu_name))
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
plt.savefig(output_file_path)
