#!/usr/bin/env -S mawk -f

BEGIN {
  FS = ";"
}

{
  part_min = $2 + 0
  sum10[$1] += ($3 + 0)
  total[$1] += ($4 + 0)
  part_max = $5 + 0

  if (!($1 in min) || part_min < min[$1]) min[$1] = part_min
  if (!($1 in max) || part_max > max[$1]) max[$1] = part_max
}

# custom sort function because mawk doesn't have one :/
function merge_sort(keys, keys_lc, tmp_keys, tmp_lc, left, right, mid, i, j, t) {
  if (left >= right) return

  mid = int((left + right) / 2)
  merge_sort(keys, keys_lc, tmp_keys, tmp_lc, left, mid)
  merge_sort(keys, keys_lc, tmp_keys, tmp_lc, mid + 1, right)

  i = left
  j = mid + 1
  t = left

  while (i <= mid && j <= right) {
    if (keys_lc[i] < keys_lc[j] || (keys_lc[i] == keys_lc[j] && keys[i] <= keys[j])) {
      tmp_keys[t] = keys[i]
      tmp_lc[t] = keys_lc[i]
      i++
    } else {
      tmp_keys[t] = keys[j]
      tmp_lc[t] = keys_lc[j]
      j++
    }
    t++
  }

  while (i <= mid) {
    tmp_keys[t] = keys[i]
    tmp_lc[t] = keys_lc[i]
    i++
    t++
  }

  while (j <= right) {
    tmp_keys[t] = keys[j]
    tmp_lc[t] = keys_lc[j]
    j++
    t++
  }

  for (i = left; i <= right; i++) {
    keys[i] = tmp_keys[i]
    keys_lc[i] = tmp_lc[i]
  }
}

END {
  n = 0
  for (k in min) {
    n++
    keys[n] = k
    keys_lc[n] = tolower(k)
  }

  if (n > 1) {
    merge_sort(keys, keys_lc, tmp_keys, tmp_lc, 1, n)
  }

  for (i = 1; i <= n; i++) {
    k = keys[i]
    mean = sum10[k] / (total[k] == 0 ? 1 : total[k]) / 10
    printf "%s=%.1f/%.1f/%.1f\n", k, min[k], max[k], mean
  }
}
