import sys
import time
import concurrent.futures
import signal

interrupted = False

def handler(signum, frame):
    global interrupted

    # Gracefull shutfown
    print('Signal handler called with signal', signum)
    interrupted = True
    #sys.exit(-1)

def worker(fname):

    global interrupted

    if interrupted:
        print("interrupted")
        return None

    print(fname)
    #time.a(4)
    time.sleep(5)
    #except KeyboardInterrupt:
    #    print("bora")


files = ['a', 'b', 'c', 'd', 'e', 'f', 'g']

if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler)

    with concurrent.futures.ProcessPoolExecutor(max_workers=5) as executor:

        try:

            results = executor.map(worker, files)
            for res in results:
                print("DONE")

        except Exception as exc:
            print(exc)
            executor.shutdown(wait=True, cancel_futures=True)

#worker(files[0])
