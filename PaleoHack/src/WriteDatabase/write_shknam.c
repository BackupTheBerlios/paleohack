#include <stdio.h>
#include "gen-shknam.h"
#include "swap.h"

// write: 1 short == number of shoptypes
// write: n shorts == the indices of each shoptype
// n times:
//   write: 1 short == number of names
//   write: m shorts == the indices of each name within a shoptype, and end
//   write: m-1 names, don't need null terminator

int getsize_shknam()
{
  return shknam_record_size;
}

void write_shknam(FILE *fp)
{
  short tmp, i, j;
  char zero = '\0';

  // number of shop types
  tmp = SwapWord(SHOP_TYPES);
  fwrite(&tmp, 2, 1, fp);

  // for each shop type, the number of names allotted to it
  for (i = 0 ; i < SHOP_TYPES ; i++) {
    tmp = SwapWord(shknam_name_counts[i]);
    fwrite(&tmp, 2, 1, fp);
  }
  // offsets
  for (j = 0 ; j < MAX_SHK_NAMES + 1 ; j++) {
    tmp = SwapWord(shknam_name_offsets[j]);
    fwrite(&tmp, 2, 1, fp);
  }
  // names
  for (j = 0 ; j < MAX_SHK_NAMES ; j++) {
    fprintf(fp, "%s", shknam_names[j]);
    // no null terminator!
  }
}
