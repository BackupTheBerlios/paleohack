#include <stdio.h>
#include "gen-rumors.h"
#include "swap.h"

// 1 short == number of rumors
// n shorts == the offset of each rumor
// 1 short == the end of the last rumor
// n strings == the rumors


int getsize_rumors()
{
  return rumors_offsets[MAX_RUMORS];
}

void write_rumors(FILE *fp)
{
  short tmp, i, j;
  char zero = '\0';

  tmp = SwapWord(MAX_RUMORS);
  fwrite(&tmp, 2, 1, fp);

  // offsets
  for (j = 0 ; j < MAX_RUMORS + 1 ; j++) {
    tmp = SwapWord(rumors_offsets[j]);
    fwrite(&tmp, 2, 1, fp);
  }
  // strings
  for (j = 0 ; j < MAX_RUMORS ; j++) {
    fprintf(fp, "%s", rumors[j]);
    fwrite(&zero, 1, 1, fp);//null terminated - convenient but wastes 505 bytes
  }
}
