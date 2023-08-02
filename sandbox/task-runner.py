import subprocess
from concurrent.futures import ThreadPoolExecutor

import time
import tqdm


files = ['a', 'b', 'c', 'd', 'e', 'f']

def do_task(filename):
    cmd = ["sleep", '10'] 

    p = subprocess.run(cmd)
    print(p.returncode)

    time.sleep(5)


nprocesses = 2
if __name__ == '__main__':

    with ThreadPoolExecutor(max_workers=nprocesses) as pool:
        with tqdm.tqdm(total=len(files)) as progress:
            futures = []

            for f in files:
                future = pool.submit(do_task, f)
                future.add_done_callback(lambda p: progress.update())
                futures.append(future)

            results = []
            
            for f in futures:
                res = f.result()
                results.append(res)

