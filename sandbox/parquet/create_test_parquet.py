#!/usr/bin/env python3
"""Create a test Parquet file for testing minparquet reader."""

import pyarrow as pa
import pyarrow.parquet as pq

# Create test data
data = {
    'id': [1, 2, 3, 4, 5],
    'name': ['Alice', 'Bob', 'Charlie', 'Diana', 'Eve'],
    'score': [95.5, 87.3, 92.1, 88.7, 91.2],
    'active': [True, False, True, True, False],
    'count': [100, 200, 150, 175, 225],
}

table = pa.table(data)

# Write with no compression (easiest to test)
pq.write_table(table, 'test_uncompressed.parquet', compression=None)
print("Created: test_uncompressed.parquet")

# Write with GZIP compression
try:
    pq.write_table(table, 'test_gzip.parquet', compression='gzip')
    print("Created: test_gzip.parquet")
except Exception as e:
    print(f"GZIP failed: {e}")

# Write with ZSTD compression
try:
    pq.write_table(table, 'test_zstd.parquet', compression='zstd')
    print("Created: test_zstd.parquet")
except Exception as e:
    print(f"ZSTD failed: {e}")

# Write with Snappy compression (common default)
try:
    pq.write_table(table, 'test_snappy.parquet', compression='snappy')
    print("Created: test_snappy.parquet")
except Exception as e:
    print(f"Snappy failed: {e}")

print("\nTest files created successfully!")
