import decode
import seaborn as sns
import sys
import matplotlib.pyplot as plt

if __name__ == '__main__':
    arr = decode.read_gtif(sys.argv[3]) - decode.decompress(sys.argv[1], int(sys.argv[2]))

    sns.heatmap(arr, linewidths=0)
    plt.show();
