# (m)awk + GNU Parallel

Just being silly and seeing how fast "standard" data processing utilities can be.

Uses [mawk](https://invisible-island.net/mawk/) for processing and [parallel](https://www.gnu.org/software/parallel/) for spliting the input across different processes.

To run it use the `run.sh` script:

```
$ ./run.sh /path/to/measurements.txt
```
