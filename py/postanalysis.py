import decode, sys, os
import subprocess, time
import numpy as np
from prettytable import PrettyTable

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: original.tif compressed.gpgc")
        exit(1)

    output = PrettyTable()

    ori_file = sys.argv[1]
    comp_file = sys.argv[2]
    start = time.time()
    result = subprocess.run(["./Debug/gpgcconv", comp_file, "GPGC_BENCHMARK"], stdout=subprocess.PIPE)
    end = time.time()

    original = np.array(decode.read_gtif(ori_file))
    compressed = np.array(decode.decompress(comp_file + ".log", 512))
    diffs = abs((original - compressed)).flatten()

    comp_ratio = (os.stat(ori_file).st_size) / (os.stat(comp_file).st_size)
    output.field_names = ["Field", "Value"]
    output.add_row([ "Comp. Ratio", round(comp_ratio, 5) ])
    output.add_row([ "Error Stddev", round(diffs.std(), 5)])
    output.add_row([ "Mean Avg. Error", round(diffs.mean(), 5) ])
    output.add_row([ "RMSE", round(np.sqrt((diffs ** 2).mean()), 5) ])
    output.add_row([ "Decoding Time", round(end-start, 7) ])
    output.align = "l"

    print(output)
    
