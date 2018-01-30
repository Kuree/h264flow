import threading
import multiprocessing
import time
import argparse
import os
from shutil import copyfile
from subprocess import Popen


def get_args():
    parser = argparse.ArgumentParser("h264flow benchmark")
    parser.add_argument("-i", action="store", dest="input", required=True,
                        help="input file")
    return parser.parse_args()


def run_instance(*filename):
    filename = str("".join(filename))
    if not os.path.isfile(filename):
        print("ERROR:", filename, "does not exist")
        return
    p = Popen(["./benchmark", "-i", filename])
    p.wait()


def main():
    filename = get_args().input
    benchmark_dir = "benchmark_dir"
    if not os.path.isdir(benchmark_dir):
        os.makedirs(benchmark_dir)

    if not os.path.isfile(filename):
        print(filename, "does not exist")
        exit(1)

    core_count = multiprocessing.cpu_count()
    file_list = []
    for i in range(core_count):
        new_name = os.path.join(benchmark_dir, str(i) + ".mp4")
        copyfile(filename, new_name)
        file_list.append(new_name)

    start = time.time()
    threads = [threading.Thread(target=run_instance, args=cmd) for cmd in
               file_list]
    [p.start() for p in threads]
    [p.join() for p in threads]
    end = time.time()
    time_used = end - start
    print("Total time used", time_used)


if __name__ == "__main__":
    main()
