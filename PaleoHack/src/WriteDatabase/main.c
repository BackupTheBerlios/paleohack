#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "write_objclass.h"
#include "write_permonst.h"
#include "write_trobj.h"
#include "write_shknam.h"
#include "write_rumors.h"
#include "records.h"
#include "swap.h"

// objclass names, objclass descrs, objclass = 3
// permonst names, permonst = 2
// trobj = 1 (running total: 6)
// shopkeeper names = 1 (running total: 7)
#define NUM_RECS 8

// arbitrary
#define UID_START 64


// hit C with the Object Oriented ugly-stick:
typedef struct {
  int (*getsize)();
  void (*write)(FILE *);  
  unsigned long uid;
} recwriter_t;
// this will make things less painful, but more cryptic, below.
recwriter_t r_tab[NUM_RECS] = {
  { getsize_objclass,        write_objclass,        REC_OBJCL },
  { getsize_objclass_names,  write_objclass_names,  REC_OBJCL_NAME },
  { getsize_objclass_descrs, write_objclass_descrs, REC_OBJCL_DESC },
  { getsize_permonst,        write_permonst,        REC_PERMON },
  { getsize_permonst_names,  write_permonst_names,  REC_PERMON_NAME },
  { getsize_trobj,           write_trobj,           REC_TROBJ },
  { getsize_shknam,          write_shknam,          REC_SHKNAM },
  { getsize_rumors,          write_rumors,          REC_RUMORS },
};


// all numbers in palmos db records are big-endian
// so if you're writing anything other than chars, call SwapWhatever first.
typedef struct {
  char sName[32];               // 32 bytes
  unsigned long unknown1;       // 36
  unsigned long time1;          // 40
  unsigned long time2;          // 44
  unsigned long time3;          // 48
  unsigned long lastSync;       // 52
  unsigned long ofsSort;        // 56
  unsigned long ofsCategories;  // 60
  unsigned long type;           // 68
  unsigned long creator;        // 64
  unsigned long unknown2;       // 72
  unsigned long unknown3;       // 76
  unsigned short numRecs;       // 78
} header_t;
// make sure that the compiler doesn't pad to multiple of 4
#define HEADER_SIZE 78
#define PREAMBLE_SIZE 80

void write_preamble(FILE *fout)
{
  header_t h;
  h.unknown1 = h.unknown2 = h.unknown3 = 0;
  h.time3 = h.lastSync = h.ofsSort = h.ofsCategories = 0;
  strncpy(h.sName, "PaleoHackDB", 32);
  h.sName[31] = '\0'; // paranoia
  strncpy((char *) &h.time1, "\x06\xD1\x44\xAE", 4); // magic.
  h.time2 = h.time1;
  strncpy((char *) &h.type, "Data", 4);
  strncpy((char *) &h.creator, "PlHk", 4);
  h.numRecs = SwapWord(NUM_RECS);
  //fwrite(&h, HEADER_SIZE, 1, fout); // or is it 1, HEADER_SIZE ?
  fwrite(&h, 1, HEADER_SIZE, fout); // or is it 1, HEADER_SIZE ?
}


void write_offsets(FILE *fout)
{
  int i;
  unsigned long offset;
  unsigned long datasize;
  unsigned long uid_etc; 
  unsigned long bigend;
  unsigned short zero = 0;

  offset = HEADER_SIZE;
  offset += 2 * sizeof(unsigned long) * NUM_RECS;
  offset += sizeof(unsigned short);

  for (i = 0 ; i < NUM_RECS ; i++) {
    uid_etc = r_tab[i].uid | 0x40000000; // the pattern for attributes=dirty ?
    printf("offset %lx     ", offset);
    printf("writing uid %lx\n", uid_etc);

    // write offset and uid_etc ...  in that order, i think ...
    bigend = SwapLong(offset);
    fwrite(&bigend, sizeof(unsigned long), 1, fout);
    bigend = SwapLong(uid_etc);
    fwrite(&bigend, sizeof(unsigned long), 1, fout);

    datasize = (*(r_tab[i].getsize))();
    //    datasize += (4 - datasize % 4); // probably not needed
    printf("size %ld\n", datasize);
    offset += datasize;
  }
  // and we need one more word.  it's zero, no need to swap it
  fwrite(&zero, sizeof(unsigned short), 1, fout);
}


// yee, haw.
void write_guts(FILE *fout)
{
  int i;

  for (i = 0 ; i < NUM_RECS ; i++) {
    (*(r_tab[i].write))(fout);
  }
}


int
main(int argc, char** argv)
{
  FILE *fout;

  if ( ! SwapChoose()) {
    printf("\nfailed to select proper byte swapping algorithm\n");
    exit(1);
  }

  if (argc < 2) {
    printf("usage: %s fooDB.pdb\n", argv[0]);
    exit(2);
  }

  fout = fopen(argv[1], "wb");
  if (fout==0) {
    printf("can't open %s for read/write\n", argv[1]);
    exit(2);
  }

  write_preamble(fout);
  write_offsets(fout);
  write_guts(fout);
  fclose(fout);
  return 0;
}
