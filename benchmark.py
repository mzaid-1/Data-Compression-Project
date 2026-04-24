import os
import subprocess
import time
import csv
import re

BENCHMARK_DIR = "./benchmarks"
RESULTS_DIR = "./results"
EXECUTABLE = "./bzip2_impl"
CONFIG_FILE = "config.ini"
RESULTS_CSV = "results.csv"

def get_file_size(filepath):
    return os.path.getsize(filepath)

def run_command(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout, result.stderr

def parse_metrics(output):
    # Example: "Done: 91 -> 101 bytes (111.0%) 0.3 ms"
    match = re.search(r"Done: (\d+) -> (\d+) bytes\s+\(([\d.]+)%\)\s+([\d.]+) ms", output)
    if match:
        orig = int(match.group(1))
        comp = int(match.group(2))
        ratio = float(match.group(3))
        time_ms = float(match.group(4))
        return orig, comp, ratio, time_ms
    return None

def main():
    if not os.path.exists(RESULTS_DIR):
        os.makedirs(RESULTS_DIR)

    files = [f for f in os.listdir(BENCHMARK_DIR) if os.path.isfile(os.path.join(BENCHMARK_DIR, f))]
    
    with open(RESULTS_CSV, mode='w', newline='') as csvfile:
        fieldnames = ['File', 'Size', 'BlockSize', 'CompressionRatio', 'Time', 'Memory']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()

        for filename in files:
            infile = os.path.join(BENCHMARK_DIR, filename)
            outfile = os.path.join(RESULTS_DIR, filename + ".bz2p1")
            
            print(f"Benchmarking {filename}...")
            
            # Use a default block size if not detectable (usually 500000 in config.ini)
            block_size = 500000 
            
            stdout, stderr = run_command(f"{EXECUTABLE} compress {infile} {outfile} {CONFIG_FILE}")
            metrics = parse_metrics(stdout)
            
            if metrics:
                orig, comp, ratio, time_ms = metrics
                writer.writerow({
                    'File': filename,
                    'Size': orig,
                    'BlockSize': block_size,
                    'CompressionRatio': ratio,
                    'Time': time_ms,
                    'Memory': "N/A" # Estimation of peak memory usage (implied or measured)
                })
            else:
                print(f"Failed to parse metrics for {filename}")

    print(f"Results saved to {RESULTS_CSV}")

if __name__ == "__main__":
    main()
