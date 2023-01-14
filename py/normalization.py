import decode
import sys
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

if __name__ == '__main__':
    dcd = np.array(decode.decompress(sys.argv[1], int(sys.argv[2])))
    ori = np.array(decode.read_gtif(sys.argv[3]))

    err = dcd - ori
    err_f = err.flatten()

    x = stats.kstest(err_f, "norm")

    print(x)
    plt.hist(err_f, bins=np.arange(-20, 20, 1))
    plt.show()

