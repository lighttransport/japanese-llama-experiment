#!/usr/bin/env python # -*- coding: utf-8 -*-
# author      : Chenghao Mou (mouchenghao@gmail.com)
# created     : 10/4/22
# Modification : Light Transport Entertainment, Inc.
from __future__ import annotations

from io import BytesIO, StringIO
import argparse
import gc
import multiprocessing as mp
import os
import pickle
import random
import re
from collections import defaultdict
from typing import Any
from typing import Callable
from typing import Dict
from typing import List
from typing import Set
from typing import Tuple

import logging

#from rich.logging import RichHandler

logger = logging.getLogger("text_dedup")
logger.setLevel(logging.INFO)

ch = logging.StreamHandler()
ch.setLevel(logging.INFO)

logger.addHandler(ch)

#logger.addHandler(RichHandler(rich_tracebacks=True))
#logger.propagate = False

#import datasets
import numpy as np
#from datasets import load_dataset
#from datasets import load_from_disk
from tqdm import tqdm

#from . import logger
from utils import UnionFind
from utils import ngrams
from utils.add_args import add_io_args
from utils.add_args import add_meta_args
from utils.add_args import add_minhash_args
from utils.analysis import optimal_param
from utils.hashfunc import sha1_hash
from utils.hashfunc import xxh3_16hash
from utils.hashfunc import xxh3_32hash
from utils.timer import Timer
from utils.data_loadsave import load_json_files, load_file, save_file
from utils.preprocess import normalize as normalize_for_dedup

SEED = 42
RNG = np.random.RandomState(SEED)
NON_ALPHA = re.compile("\W", re.UNICODE)
#datasets.logging.set_verbosity_error()


def get_nprocs():
    return min(os.cpu_count()//4, 8)

def embed_func(
    content: str,
    idx: int,
    *,
    num_perm: int,
    ngram_size: int,
    min_length: int,
    hashranges: List[Tuple[int, int]],
    permutations: np.ndarray,
    hash_func: Callable,
    dtype: type,
    max_hash: np.uint,
    modulo_prime: np.uint,
) -> Dict[str, Any]:
    """
    Calculate hash values for the content.

    Parameters
    ----------
    content : str
        The content to be embedded.
    idx : int
        The index of the content.
    num_perm : int
        The number of permutations.
    ngram_size : int
        The size of n-grams.
    min_length : int
        The minimum length of the document in terms of tokens.
    hashranges : List[Tuple[int, int]]
        The ranges of hash values.
    permutations : np.ndarray
        The permutations for the minhash.
    hash_func : Callable
        The hash function to use.

    Returns
    -------
    Dict[str, Any]
        The hash values in each range and the index.

    Examples
    --------
    >>> content = "hello world"
    >>> idx = 0
    >>> num_perm = 250
    >>> ngram_size = 1
    >>> hashranges = [(i, i + 25) for i in range(0, 250, 25)]
    >>> max_hash = np.uint32((1 << 32) - 1)
    >>> modulo_prime = np.uint32((1 << 32) - 5)
    >>> PERMUTATIONS = np.array(
    ...     [
    ...         (
    ...             RNG.randint(1, np.uint32((1 << 32) - 5), dtype=np.uint32),
    ...             RNG.randint(0, np.uint32((1 << 32) - 5), dtype=np.uint32),
    ...         )
    ...         for _ in range(num_perm)
    ...     ],
    ...     dtype=np.uint32,
    ... ).T
    >>> res = embed_func(content, idx, num_perm=num_perm, ngram_size=ngram_size, min_length=0, hashranges=hashranges,
    ... permutations=PERMUTATIONS, hash_func=xxh3_32hash,dtype=np.uint32, max_hash=max_hash, modulo_prime=modulo_prime)
    >>> len(res["__signatures__"])
    10
    >>> res["__id__"]
    0
    """

    normalized_content = normalize_for_dedup(content)
    #normalized_content = content
    a, b = permutations
    tokens: Set[bytes] = {
        bytes(" ".join(t).lower(), "utf-8") for t in ngrams(NON_ALPHA.split(normalized_content), ngram_size, min_length)
    }
    for tok in tokens:
        print(tok.decode('utf-8'))

    hashvalues: np.ndarray = np.array([hash_func(token) for token in tokens], dtype=dtype)
    # Permute the hash values to produce new universal hashes
    # Tiling 'a' to match the shape of 'hashvalues'
    # Element-wise multiplication of 'hashvalues' with tiled 'a'
    # Adding 'b' and taking the result modulo 'MERSENNE_PRIME'
    # Performing bitwise AND with 'MAX_HASH'
    hashvalues = np.bitwise_and(
        np.mod(np.add(np.multiply(hashvalues, np.tile(a, (len(hashvalues), 1)).T).T, b), modulo_prime),
        max_hash,
    )
    # this part is where the name "min" of minhash comes from
    # this stacks all the hashes and then takes the minimum from each column
    masks: np.ndarray = np.full(shape=num_perm, dtype=dtype, fill_value=max_hash)
    hashvalues = np.vstack([hashvalues, masks]).min(axis=0)
    # Originally, byteswap was done for speed. Testing show it has a negligible impact
    # keeping  for backward compatibility, even though theoretically and empirically
    # it doesnt matter if it is there or not. github.com/ekzhu/datasketch/issues/114
    Hs = [bytes(hashvalues[start:end].byteswap().data) for start, end in hashranges]
    return {"__signatures__": Hs, "__id__": idx}


if __name__ == "__main__":  # pragma: no cover
    parser = argparse.ArgumentParser(
        prog="text_dedup.minhash",
        description="Deduplicate text using minhash",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser = add_io_args(parser)
    parser = add_meta_args(parser)
    parser = add_minhash_args(parser)
    args = parser.parse_args()

    HASH_BITS: int = args.hash_bits

    # mypy typing with numpy is difficult
    # 64 bit config is backwards compatibility mode.
    # it uses 64 bit types but almost entirely 32bit data, except for one mersenne prime 2^61
    # why legacy implementations used mersenne primes for modulo:
    # https://en.wikipedia.org/wiki/Universal_hashing#Hashing_strings
    HASH_CONFIG: Dict[int, Tuple[type, Any, Any]] = {
        64: (np.uint64, np.uint64((1 << 32) - 1), np.uint64((1 << 61) - 1)),
        # 32, 16 bit config does not use a mersenne prime.
        # The original reason for using mersenne prime was speed.
        # Testing reveals, there is no benefit to using a 2^61 mersenne prime for division
        32: (np.uint32, np.uint32((1 << 32) - 1), np.uint32((1 << 32) - 5)),
        16: (np.uint16, np.uint16((1 << 16) - 1), np.uint16((1 << 16) - 15)),
    }

    # defaults to backwards compatible HASH_BITS = 64, which is np.uint64 dtypes with 32bit hashes
    DTYPE, MAX_HASH, MODULO_PRIME = HASH_CONFIG.get(HASH_BITS, HASH_CONFIG[64])

    match args.hash_func:
        case "sha1":

            def hash_func(byte_data):
                return sha1_hash(byte_data, d=min(HASH_BITS, 32))

        case "xxh3":
            if HASH_BITS == 16:
                hash_func = xxh3_16hash
            else:
                hash_func = xxh3_32hash

    mp.set_start_method("fork", force=True)
    uf = UnionFind()
    timer = Timer()

    if args.b is not None and args.r is not None:
        B, R = args.b, args.r
    else:
        # Compute the optimal `MinHashLSH` parameter that minimizes the weighted sum
        # of probabilities of false positive and false negative, taken from datasketch.
        # You can also refer to the interactive demo at https://huggingface.co/spaces/bigcode/near-deduplication.
        # The following assumes a "perfect hash". using 16 bit hashes might challenge this assumption
        # lower precision dtype will cause more collisions, so higher false_positives and less false negatives.
        # Both effects move the result towards more documents being considered duplicates.
        B, R = optimal_param(args.threshold, args.num_perm, false_positive_weight=0.5, false_negative_weight=0.5)

    HASH_RANGES = [(i * R, (i + 1) * R) for i in range(B)]
    HASH_TABLES: List[Dict[int, Set]] = [defaultdict(set) for _ in range(B)]

    with timer("Total"):
        with timer("Loading"):
            #if args.local:
            #    ds = load_from_disk(args.path)
            #else:
            #    ds = load_dataset(
            #        path=args.path,
            #        name=args.name,
            #        data_dir=args.data_dir,
            #        data_files=args.data_files,
            #        split=args.split,
            #        revision=args.revision,
            #        cache_dir=args.cache_dir,
            #        num_proc=os.cpu_count(),
            #        token=args.use_auth_token,
            #    )

            if args.data_files:
                # load multiple json files(assume data_files contains glob pattern
                ds = load_json_files(args.data_files, args.offset, args.nfiles)
            else:
                # load single file
                ds = load_file(args.file)

        DATA_SIZE = len(ds)
        print("# of items:", DATA_SIZE)
        PERMUTATIONS: np.ndarray = np.array(
            [
                (
                    RNG.randint(1, MODULO_PRIME, dtype=DTYPE),
                    RNG.randint(0, MODULO_PRIME, dtype=DTYPE),
                )
                for _ in range(args.num_perm)
            ],
            dtype=DTYPE,
        ).T

        with timer("MinHashing"):
            embedded = ds.map(
                function=embed_func,
                fn_kwargs={
                    "num_perm": args.num_perm,
                    "hashranges": HASH_RANGES,
                    "ngram_size": args.ngram,
                    "min_length": args.min_length,
                    "permutations": PERMUTATIONS,
                    "hash_func": hash_func,
                    "dtype": DTYPE,
                    "max_hash": MAX_HASH,
                    "modulo_prime": MODULO_PRIME,
                },
                input_columns=[args.column],
                remove_columns=ds.column_names,
                num_proc=get_nprocs(),
                with_indices=True,
                desc="Fingerprinting...",
            )

        with timer("Clustering"):
            for i in tqdm(
                range(0, len(embedded), args.batch_size),
                dynamic_ncols=True,
                desc="Iterating MinHashes...",  # noqa: E501
            ):
                batch = embedded[i : i + args.batch_size]
                for key, Hs in zip(batch["__id__"], batch["__signatures__"]):
                    for i, H in enumerate(Hs):
                        HASH_TABLES[i][H].add(key)

            for table in tqdm(HASH_TABLES, dynamic_ncols=True, desc="Clustering..."):
                for cluster in table.values():
                    if len(cluster) <= 1:
                        continue
                    idx = min(cluster)
                    for x in cluster:
                        uf.union(x, idx)

        with timer("Filtering"):
            gc.freeze()
            gc.disable()
            ds = ds.map(
                function=lambda _, idx: {"__cluster__": uf.find(idx)},
                with_indices=True,
                num_proc=get_nprocs(),
                new_fingerprint=str(random.getrandbits(128)),
                desc="Finding clusters...",
            )
            gc.enable()
            gc.collect()
            # This is where the deduplication happens
            # Since there is no easy groupby in datasets
            # I will use this simple filter for now
            final_data = ds.filter(
                function=lambda record, idx: record["__cluster__"] == idx,
                with_indices=True,
                num_proc=get_nprocs(),
                desc="Filtering clusters...",
            )

        with timer("Saving"):
            final_data = final_data.remove_columns(["__cluster__"])

            buf = BytesIO()
            nbytes = final_data.to_json(buf)
            save_file(buf.getbuffer(), args.output)
            #final_data.save_to_disk(args.output)
            if args.debug:
                with open(os.path.join(args.output, "uf.pkl"), "wb") as f:
                    pickle.dump(uf, f, protocol=pickle.HIGHEST_PROTOCOL)

        with timer("Cleaning"):
            if args.clean_cache:
                ds.cleanup_cache_files()

    PAD = 32
    for k, v in timer.elapsed_times.items():
        logger.info(f"{k:<{PAD}}: {v:.2f}s")

    logger.info(f"{'Before':<{PAD}}: {len(ds)}")
    logger.info(f"{'After':<{PAD}}: {len(final_data)}")
