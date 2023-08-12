import subprocess

nfiles = 32
cnt = 0
for i in range(0, 1001, 32):
    iend = i + nfiles
    iend = min(1001, iend)
    n = iend - i

    infilename = "../data/02_clean_step1/cc100ja/cc100-ja.{:05d}.jsonl.zstd"
    outfilename = "../data/03_dedup/cc100ja/cc100-ja.dedup.{:05d}.jsonl.zstd".format(cnt)
    column_name = 'text'

    cmd = ['python', 'text_dedup/cc100ja.py', '--data_files', infilename,
        '--offset', str(i), '--nfiles', str(n), '--output', outfilename, '--column', column_name]

    print(cmd)
    ret = subprocess.run(cmd)

    cnt += 1
