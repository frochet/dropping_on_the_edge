import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import sys
import pdb
import seaborn as sns
#sns.set(color_codes=True)
sns.set_style(style='white')

def parse_web_data(pathname):
    starting_point = 0
    x_y = {}
    with open(pathname, "r") as f:
        for line in f:
            if "COUNTER" in line:
                if starting_point == 0:
                    starting_point = line.split("COUNTER STARTS.")[1].split('.')[0].split(':')
                x = line.split("COUNTER STARTS.")[1].split('.')[1][:-1]
                x_y[x] = [line.split("COUNTER STARTS.")[1].split('.')[0].split(':')] # give [second, nanosecond]
            elif "relay_send_command_from_edge" in line:
                x = line.split(' ')[-1].split('.')[0]
                if (len(x_y[x]) < 3): #We just keep track of the 3 first timing - when we start resolving; when we package a CONNECTED CELL and when we package the first DATA cell
                    x_y[x].append(line[:-1].split(' ')[-1].split('.')[1].split(':'))
    
    connected_cells = []
    first_data_cells = []
    values = x_y.values()
    for items in values:
        if len(items) == 3: #if not, the connection is incomplete. Meaning that something shitty happened with this circuit. We ignore it
            connected_cells.append(int(items[1][0])-int(items[0][0]) + (float(items[1][1])-float(items[0][1]))/1000000000.0)
            first_data_cells.append(int(items[2][0])-int(items[0][0]) + (float(items[2][1])-float(items[0][1]))/1000000000.0)
    return connected_cells, first_data_cells


def web_plot(connected_cells, first_data_cells, filename):
    fig, ax = plt.subplots(nrows=1, ncols=1)

    sns.distplot(connected_cells, hist=False, rug=True, ax=ax, label="Sends back the CONNECTED cell");
    sns.distplot(first_data_cells, hist=False, rug=True, ax=ax, label="Sends back the first cell of the data flow");
    ax.set_xlim([0,0.5]) #lim to 500ms
    ax.set_xlabel('seconds since call to dns_resolve()', fontsize=18)
    ax.set_ylabel('density', fontsize=18)
    ax.legend(fontsize=14)
    fig.savefig('dist_conn_cells_first_data_cells_{0}.png'.format(filename))
    plt.close(fig)
if __name__ ==  "__main__":

    if sys.argv[1] == "webbarchart":
        pathname = sys.argv[2]
        connected_cells, first_data_cells = parse_web_data(pathname)
        pdb.set_trace()
        web_plot(connected_cells, first_data_cells, sys.argv[3])

