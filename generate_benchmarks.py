#!/usr/bin/env python3
"""
generate_benchmarks.py
Generates several sample test files in the benchmarks/ directory.
Run once before testing: python3 generate_benchmarks.py
"""
import os, random, string

os.makedirs("benchmarks", exist_ok=True)
os.makedirs("results",    exist_ok=True)

# 1. Text file with repetitive content (good BWT candidate)
with open("benchmarks/text_repetitive.txt", "w") as f:
    sentence = "the quick brown fox jumps over the lazy dog\n"
    f.write(sentence * 2000)
print("Created benchmarks/text_repetitive.txt")

# 2. Text file with English-like content
words = ["the","and","or","of","in","to","a","is","it","that","was","for",
         "on","are","with","as","at","be","this","have","from","by","not"]
with open("benchmarks/text_english.txt", "w") as f:
    for _ in range(5000):
        f.write(" ".join(random.choices(words, k=10)) + "\n")
print("Created benchmarks/text_english.txt")

# 3. High-repetition binary file
with open("benchmarks/binary_repeat.bin", "wb") as f:
    pattern = bytes([0xAA, 0xBB, 0x00, 0x00, 0x00, 0xFF])
    f.write(pattern * 15000)
print("Created benchmarks/binary_repeat.bin")

# 4. Random binary (incompressible — tests graceful handling)
with open("benchmarks/binary_random.bin", "wb") as f:
    f.write(bytes(random.randint(0, 255) for _ in range(50000)))
print("Created benchmarks/binary_random.bin")

# 5. Small text for quick manual verification
with open("benchmarks/small.txt", "w") as f:
    f.write("banana\n")
    f.write("abracadabra\n")
    f.write("mississippi\n")
    f.write("AAABBBCCCDDD\n")
print("Created benchmarks/small.txt")

# 6. Single character repeated (extreme RLE case)
with open("benchmarks/single_char.txt", "w") as f:
    f.write("A" * 10000)
print("Created benchmarks/single_char.txt")

print("\nAll benchmark files ready in ./benchmarks/")
