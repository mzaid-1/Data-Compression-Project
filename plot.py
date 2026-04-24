import pandas as pd
import matplotlib.pyplot as plt
import os

def generate_graphs(csv_file):
    if not os.path.exists(csv_file):
        print(f"Error: {csv_file} not found.")
        return

    df = pd.read_csv(csv_file)
    
    # 1. Compression Ratio by File
    plt.figure(figsize=(10, 6))
    plt.bar(df['File'], df['CompressionRatio'], color='skyblue')
    plt.axhline(y=100, color='r', linestyle='--', label='Original Size')
    plt.ylabel('Compression Ratio (%)')
    plt.title('Compression Ratio by File (Phase 1)')
    plt.xticks(rotation=45)
    plt.legend()
    plt.tight_layout()
    plt.savefig('results/compression_ratio.png')
    
    # 2. Compression Time by File Size
    plt.figure(figsize=(10, 6))
    plt.scatter(df['Size'] / 1024, df['Time'], color='green')
    plt.xlabel('File Size (KB)')
    plt.ylabel('Time (ms)')
    plt.title('Compression Time vs File Size')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig('results/performance_time.png')
    
    print("Graphs generated in results/ directory.")

if __name__ == "__main__":
    generate_graphs('results.csv')
