import matplotlib.pyplot as plt
import csv
import numpy as np


def readCSV(file_name, delimiter=','):
    file = csv.DictReader(open(file_name), delimiter=delimiter)
    return list([row for row in file])


instances = [row['instance'] for row in readCSV(
    'results/dijkstra/dimacs/melon_static_digraph.csv')]
libs = ["BGL_adjacency_list_vecS", "BGL_compressed_sparse_row_graph",
        "LEMON_StaticDigraph", "MELON_static_digraph"]
data = [
    [int(float(row['time_ms']))
     for row in readCSV('results/dijkstra/dimacs/bgl_adjacency_list_vecS.csv')],
    [int(float(row['time_ms']))
     for row in readCSV('results/dijkstra/dimacs/bgl_compressed_sparse_row.csv')],
    [int(float(row['time_ms']))
     for row in readCSV('results/dijkstra/dimacs/lemon_static_digraph.csv')],
    [int(float(row['time_ms']))
     for row in readCSV('results/dijkstra/dimacs/melon_static_digraph.csv')]
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
    'Dijkstra average runtime on USA-road instances (from 9th DIMACS challenge)')
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

plt.savefig('results/dijkstra/dimacs/plot.png')
