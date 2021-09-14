# Garbage Collector Design Notes

## LSC Garbage Collector

* A "stop the world" serial garbage collector.
* Uses power-of-2 size buckets.

## V8 Garbage Collector

* Generational, based on the "most objects die young" observation.
  * "scavenging" gc on young pages when the page is full
* Does block the main thread but has several worker threads that handle mark/sweep on their own but also can
defer work for the main thread.
* Pointer rewriting for copied objects.
* Super-large objects get their own allocations
* Four size buckets:
  * for small regions < 256 words
  * for medium regions < 2048 words
  * for large regions < 16384 words
  * XL objects get their own mmapd region per-object
* Write barrier for object marking
* 1 MB, or 512KB, or possibly 256KB fixed page size

## Bacon 2003

* "Arraylets," "stacklets," other large object allocations broken into smaller pieces for more tractable management.
* Double pointer for copied objects on a read barrier
* Much more granular buckets at s(1 + 1/p) with p = 8
* 16 KB pages, one size bucket per page, free lists are per-page

## Considerations for Hadron

* Manage some pages separately for JIT code generation, with separate mmap flags
* Incremental GC on main thread so we can block main thread as little as possible
* Generational seems attractive due to the "die young" insight
  * Class library compilation may require a way to mark objects automatically as older
* "Stacklets" is attractive for monitoring of stack growth, possibly segmenting stack sweeping into worker threads, and
  we need a memory management solution for the stack anyway
  * Will require compiler to track stack "high water mark" within a callable - max # of arguments pushed, etc
* "Arraylets" possibly attractive for same reasons but doubles cost of array access
* Allocation means a trampoline out to C++ code (this was likely inevitable)
* Things that may benefit from special treatment:
  * globals
  * symbols (maybe always old?)
  * JIT bytecode
  * compiled class library code
  * IdentityDictionaries

Approach - spaces division seems useful, given the different handling approaches required.

