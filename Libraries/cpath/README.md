# CPath

![](https://github.com/BraedonWooding/cpath/workflows/CI/badge.svg)

> Made By Braedon Wooding

> Fast (see benchmarks at the bottom), efficient (optimised towards reducing dynamic allocations), and simple

A simple and nice looking cross platform C FileSystem.  I'll put up some documentation and tests up soon.

It worked out really nicely from my basic idea in my head.  Check out the `example.c` file if you want some basic idea of how it works.

## Why

Because I wanted a filesystem for my IMGUI Widget library but I didn't want to only support C++17 so I needed a preferably early standardised C++ (or C) library.  While there existed plenty of variants their APIs weren't particularly nice or they didn't try to reduce includes or various other issues.  The key thing for me was wanting to just have a single while loop function to both check if there is a next one and then assign the next one rather than having to split it into potentially 3! Which some had (i.e. a hasNext, a getFile, and a moveNext).

I also felt that the code was quite obfuscated for some and it felt like they overused the #ifdefs to try to get as much similar as possible rather than trying to just make it more readable at the cost of a small amount of local duplication.

I chose to make this very young i.e. C89/C99 ish since I was already going quite far with making it work with all versions of C++.

## What this aims to be and what it doesn't aim to be

- A simple and straightforward lite filesystem.
- The ability to create abstract paths and canonicalise them into real ones
- Very lite cpp bindings for C++ pre 17 (does include operators however)
- Meant to be highly extensible by the user so everything is exposed
- NOT make any overarching architectural designs that could pollute the user's paradigm
- NOT a replacement for a more heavy duty library like `std::filesystem`
- NOT to dictate anything to do with opening/handling files
- NOT to dictate anything to do with stuff like watching files or creating files

## Credits

I would like to put a thanks out to a lot of other similar libraries for giving me some source to compare against to make sure I was using the APIs correctly and some ideas for how to structure my API.

## Comparisons against other libraries

> If you want performance comparisons check out the benchmarks, TLDR though is that CPath is often around 3x faster than other libraries and even faster than `find`.

CPath offers a variety of features such as;

- The ability to emplace directories and save the old directory to be able to restore it later on
  - This allows you to traverse it without having to use recursion or a custom stack (via a linked list or similar) allowing infinite traversal (within limits of RAM)
  - While TinyDir offers emplacing it won't store the old one and you can't restore it this makes recursing using it limited and requires some external stack or queue.
  - Cute files offers no such feature
- The ability to cache files and refer to them by number and then custom sort them
  - Cute files offers no such feature
  - TinyDir offers the ability to cache them but not custom sort you also can't refresh the cache
- Fully C++ Bindings in a familiar style including operators for paths
- The ability to concatenate paths together and compare them in an easy way
  - Paths seem to be fully exempt from the other libraries except as just a 'string'
- Really efficient 'open file' (that is get information about a file from a path)
  - Only requires a single system call!
  - TinyDir has a similar function but it requires opening the parent directory and finding it from the iterator, this is very prone to races and is significantly slower requiring a lot more syscalls
    - Up to O(n) minimum system calls
  - Cute files offers no such feature
- Extensive tests for all features
  - Cute files has a lot of functions marked with 'not tested' making it potentially highly unstable
  - TinyDir has few tests that test edge cases or performance but all around good coverage
- You can output file sizes using multiple different byte size representations
  - intervals of 1024 IEC (KiB, GiB, ...), JEDEC (KB, GB, ...)
  - and intervals of 1000 both in normal (kB, GB, ...) and upper/lower
  - you can also change the word `bytes` and force it to be always a word or just `B` independent of above.
  - Both tinydir and cute files offers no feature like this
- In my opinion it also looks much nicer (especially cpp bindings) you can use a single function in a while loop to iterate and its very nice
- There are some serious benchmarking work going on to make sure that we don't regress to a slower speed over time!

CPath however has a few 'cons';

- It is much larger at around ~2.5k lines compared to TinyDir's 800 and Cute Files 500
  - Cute files is much more bare bones (not a bad thing!) and results in smaller binaries
  - You can disable the CPP bindings in CPath if you want to reduce the footprint by about 600 lines
    - Just `#define NO_CPP_BINDINGS` before include
- It requires some constant tables for it's suffix printing
  - Only some small constant strings and I presume since the tables are pretty similar they'll be some optimisation or something.
- It doesn't load `stat` automatically this is mainly for optimisation purposes as it is often not needed unless you need time information or size.  It'll load it automatically as needed if you use the functions to get time / size and you can load it yourself using `cpathGetFileInfo` (or `File::GetFileInfo` in cpp) if you wish.  I wouldn't call this a con but it is something that could be an inconvenience
  - Both tinydir and cute files always load stats every time you get a file in unix
    - TinyDir also loads it in windows every time you get a file
    - Cutefiles also loads it again every time it is used for time/size

## Benchmarks

> Model Name: MacBook Air

> Model Identifier: MacBookAir7,2

> Processor Name: Intel Core i5

> Processor Speed: 1.6 GHz

> Number of Processors: 1

> Total Number of Cores: 2

> L2 Cache (per Core): 256 KB

> L3 Cache: 3 MB

> Memory: 8 GB

These were retrieved from the test file.  I sorted them by time and did a small amount of formatting.  I'll always provide a picture with each one for the purposes of comparison :).

> Note: If you feel that I somehow wrote these scripts inefficiently please submit a PR with fixes and I'll run the benchmarks.  Eventually we'll run these benchmarks on multiple machines (eventually) but for now it's easier to just not.

### Recursive vs Stack through a large directory system

> This test takes a while to run since creating the files can take a while and we are running a 100 tests to get a reasonable average.

| Test                     | User   | System | Wall   |
| ------------------------ | ------ | ------ | ------ |
| CPath (Recursion in C)   | 0.020s | 0.062s | 0.082s |
| CPath (Emplace in C)     | 0.020s | 0.063s | 0.083s |
| CPath (Recursive in cpp) | 0.020s | 0.062s | 0.082s |
| CPath (Emplace in cpp)   | 0.020s | 0.063s | 0.083s |
| find                     | 0.021s | 0.126s | 0.147s |
| Python (os.walk)         | 0.156s | 0.081s | 0.237s |
| Cute Files               | 0.041s | 0.243s | 0.284s |
| TinyDir                  | 0.050s | 0.244s | 0.294s |
| tree                     | 0.369s | 0.256s | 0.626s |

Observations

- This is done on `-O3` BUT has no difference (from what I can see) for `-O1` and `-O2` the only difference is `-O0` which is consistently around 5% slower.
- Yes this does show that CPath is faster than `find` which I'm quite proud of
  - The user time is the same which is not that surprising since they both are 'well' written in C
  - The system time is different indicating I do less syscalls
- Interestingly enough python beats both of the other libraries, I guess this comes to show that writing good code is all about optimising the bottle necks!  Even though the user time is around 4-5x faster (still not amazing since mine runs about 8-10x faster) it is absolutely hammered by the system time being around 3x slower there.  And since that is a bottle neck it results in the whole system running signifincantly slower.
- All methods are practically identical in performance for CPath.
