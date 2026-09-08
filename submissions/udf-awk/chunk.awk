#!/usr/bin/env -S mawk -f

BEGIN {
  FS = ";"
}

{
  temp = $2 + 0
  sum10[$1] += temp * 10
  total[$1] += 1
  if (!($1 in min) || temp < min[$1]) min[$1] = temp
  if (!($1 in max) || temp > max[$1]) max[$1] = temp
}

END {
  for (k in min) {
    print k ";" min[k] ";" sum10[k] ";" total[k] ";" max[k]
  }
}
