#include <stdio.h>
#include "gen-permonst.h"
#include "swap.h"

int getsize_permonst_names()
{
  // How much space we need for this record.
  int i, recsize;
  for (i = 0, recsize = 0 ; i < MAX_PERMONST_NAME ; i++)
    recsize += strlen(mon_names[i]) + 1;
  return recsize;
}
int getsize_permonst()
{
  return(sizeof(mons2));
}

void write_permonst_names(FILE *fp)
{
  int i;
  char zero = '\0';
  for (i = 0 ; i < MAX_PERMONST_NAME ; i++) {
    fprintf(fp, "%s", mon_names[i]);
    fwrite(&zero, 1, 1, fp);// null terminated - convenient but wastes 66 bytes
  }
}

void write_permonst(FILE *fp)
{
  int i;
  // will this work?  think so.  except.. need bytes swapped for shorts!  GAR.
  /*    fwrite(mons2, sizeof(permonst2), MAX_PERMONST, fp); */
  for (i = 0 ; i < MAX_PERMONST ; i++) {
    mons2[i].mname_offset  = SwapWord(mons2[i].mname_offset);
    mons2[i].pxlth         = SwapWord(mons2[i].pxlth);
  }
  fwrite(mons2, sizeof(struct permonst2), MAX_PERMONST, fp);  

}

