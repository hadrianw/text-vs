# text-vs

This is an experiment in benchmarking text editing routines.

Current version is not benchmarking anything.

To start just type:

```
$ make
```

It will:

1. download Linux kernel sources and will checkout v4.9 tag,
2. find the top 10 changed files list and save it,
3. get whole diff set for the first file and save it,
4. compile diff2edit program and run it with the diff set file

## diff2edit

The program in current version will just convert the diff
set to easier to parse format.

The final version of the program will try to produce minimal set
of editing commands in simple binary format. The resulting set would
be an aproximation of a probable session with a text editor.
Or an undo buffer.

## Benchmark

### What will be benchmarked?

- array of lines (strings) - werf text editing routines
- [librope](https://github.com/josephg/librope) based implementation
- array of lines (array of fragments) - idea similar to ropes, but using arrays instead of trees

### How?

Every implementation benchmark will load resulting editing commands set from diff2edit.

The time of the load will be measured. Then iteration through file will be measured.