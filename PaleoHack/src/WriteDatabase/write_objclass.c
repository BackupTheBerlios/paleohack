#include <stdio.h>
#include "gen-objclass.h"
#include "swap.h"

int getsize_objclass_names()
{
  // How much space we need for this record.
  int i, recsize;
  for (i = 0, recsize = 0 ; i < MAX_OBJCLASS_NAME ; i++)
    recsize += strlen(oc_names[i]) + 1;
  return recsize;
}
int getsize_objclass_descrs()
{
  // How much space we need for this record.
  int i, recsize;
  for (i = 0, recsize = 0 ; i < MAX_OBJCLASS_DESCR ; i++)
    recsize += strlen(oc_descrs[i]) + 1;
  return recsize;
}
int getsize_objclass()
{
  return(sizeof(objects2));
}


void write_objclass_names(FILE *fp)
{
  int i;
  char zero = '\0';
  for (i = 0 ; i < MAX_OBJCLASS_NAME ; i++) {
    fprintf(fp, "%s", oc_names[i]);
    fwrite(&zero, 1, 1, fp);//null terminated - convenient but wastes 203 bytes
  }
}
void write_objclass_descrs(FILE *fp)
{
  int i;
  char zero = '\0';
  for (i = 0 ; i < MAX_OBJCLASS_DESCR ; i++) {
    fprintf(fp, "%s", oc_descrs[i]);
    fwrite(&zero, 1, 1, fp);//null terminated - convenient but wastes 109 bytes
  }
}
void write_objclass(FILE *fp)
{
  int i;
  // will this work?  think so.  except.. need bytes swapped for shorts!  GAR.
  /*    fwrite(objects2, sizeof(objclass2), MAX_OBJCLASS, fp); */

  for (i = 0 ; i < MAX_OBJCLASS ; i++) {
    objects2[i].oc_name_offset  = SwapWord(objects2[i].oc_name_offset);
    objects2[i].oc_descr_offset = SwapWord(objects2[i].oc_descr_offset);
    objects2[i].oc_oi           = SwapWord(objects2[i].oc_oi);
  }
  fwrite(objects2, sizeof(struct objclass2), MAX_OBJCLASS, fp);  
}

