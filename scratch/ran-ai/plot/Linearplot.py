import numpy as np
import pandas as pd
import seaborn as sns
import tikzplotlib
import matplotlib
matplotlib.use('Agg')
matplotlib.use('Agg')
matplotlib.rcParams['mathtext.fontset'] = 'stix'
matplotlib.rcParams['font.family'] = 'STIXGeneral'
matplotlib.rcParams['font.size'] = 12
matplotlib.rcParams['figure.figsize'] = 64/9, 4
import matplotlib.pyplot as plt
plt.title(r'ABC123 vs $\mathrm{ABC123}^{123}$')


def linear_plot(distribution_data: np.ndarray,
                x_label: str,
                y_label: str,
                max_time: int,
                output_file: str,
                palette=None,
                lim: [] = None,
                plot_format='eps',
                plot_sizes=None):

    if plot_sizes is not None:
        fig = plt.figure(figsize=plot_sizes)
    else:
        fig = plt.figure()

    ax = fig.add_subplot(111)

    data_values = np.array(distribution_data, dtype=np.float32)

    value_num = len(data_values)

    data_times = (np.arange(0, value_num) * 100 / value_num).astype(int) * max_time / 100
    data_hues = ['Mean'] * len(data_values)

    data_times += 1

    data = pd.DataFrame({'Value': data_values, 'Time': data_times})

    sns.lineplot(data=data, ax=ax, x='Time', y='Value', hue=data_hues, markers=True, dashes=False, errorbar=None, palette=palette)
    # The `ci` parameter is deprecated. Use `errorbar=None` for the same effect.

    ax.grid(visible=True, color='darkgrey', linestyle='-')
    ax.grid(visible=True, which='minor', color='darkgrey', linestyle='-')

    ax.set_xlabel(x_label)

    ax.set_ylabel(y_label)

    if lim is not None:
        ax.set_ylim([lim[0], lim[1]])

    plt.savefig(output_file.replace(" ", "_") + '.png', bbox_inches='tight')

    if plot_format == 'tex':
        tikzplotlib.save(output_file.replace(" ", "_") + '.tex')

    elif plot_format is not None and plot_format != 'png':
        plt.savefig(output_file.replace(" ", "_") + '.' + plot_format, format=plot_format, bbox_inches='tight')

    plt.close()


def multi_linear_plot(distribution_data: [np.ndarray],
                      distribution_labels: [str],
                      x_label: str,
                      y_label: str,
                      max_time: int,
                      output_file: str,
                      palette=None,
                      lim: [] = None,
                      plot_format='eps',
                      plot_sizes=None):

    if plot_sizes is not None:
        fig = plt.figure(figsize=plot_sizes)
    else:
        fig = plt.figure()

    ax = fig.add_subplot(111)

    data_values = np.array([], dtype=np.float32)

    data_labels = []

    data_times = np.array([], dtype=int)  # 报错,np.int不使用，全部替换为int

    for values, label in zip(distribution_data, distribution_labels):

        value_num = len(values)

        data_values = np.concatenate((data_values, values))

        data_labels += [str(label)] * value_num

        times = (np.arange(0, value_num) * 100 / value_num).astype(int) * max_time / 100

        times += 1

        data_times = np.concatenate((data_times, times))

    data = pd.DataFrame({'Value': data_values, 'Label': data_labels, 'Time': data_times})

    plot = sns.lineplot(data=data, ax=ax, x='Time', y='Value', hue='Label', markers=True, dashes=False, errorbar=None, palette=palette)

    ax.grid(visible=True, color='darkgrey', linestyle='-')
    ax.grid(visible=True, which='minor', color='darkgrey', linestyle='-') # visible替换b，全部

    ax.legend(handles=ax.legend_.legendHandles,
              ncol=1,
              loc='upper left')

    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)

    if lim is not None:
        ax.set_ylim([lim[0], lim[1]])

    plt.savefig(output_file.replace(" ", "_") + '.png', bbox_inches='tight')

    if plot_format == 'tex':
        tikzplotlib.save(output_file.replace(" ", "_") + '.tex')

    elif plot_format is not None and plot_format != 'png':
        plt.savefig(output_file.replace(" ", "_") + '.' + plot_format, format=plot_format, bbox_inches='tight')

    plt.close()
