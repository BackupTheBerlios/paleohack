#include <stdio.h>
#include "gen-trobj.h"
#include "swap.h"

// write:  one short == number of trobj kinds
// write:  n shorts == the indices of each kind-section of the array
// write:  the structs in the array (each is 4 bytes).

int getsize_trobj()
{
  int i = 0;
  i += sizeof(short); // "MAX_TROBJ_KINDS+1"
  i += sizeof(short) * (MAX_TROBJ_KINDS+1);
  i += sizeof(trobj_t) * MAX_TROBJS;
  return i;
}

void write_trobj(FILE *fp)
{
  short tmp, i;

  tmp = SwapWord(MAX_TROBJ_KINDS+1);
  fwrite(&tmp, 2, 1, fp);

  for (i = 0 ; i < MAX_TROBJ_KINDS+1 ; i++) {
    tmp = SwapWord(indices[i]);
    fwrite(&tmp, 2, 1, fp);
  }
  for (i = 0 ; i < MAX_TROBJS ; i++) {
    fwrite(&(trobjs[i]), 1, 4, fp); // not sure if this is right.
  }

}
