#include <stdio.h>
#include <string.h>

struct permonst {
  char *mname;
  char mlet;
  char mlevel;
  char mmove;
  char ac;
  char damn;
  char damd;
  unsigned short pxlth; /* I wonder if I could just get rid of this somehow.
			   It would save... hm... 132 bytes. (big deal eh) */
};



#define SZ_ESHK    1
#define SZ_PLNAME  2
#define SZ_EGD     3
#define SZ_EDOG    4

struct permonst mons[] = {
  { "bat",                'B',1,22,8,1,4,0 },
  { "gnome",              'G',1,6,5,1,6,0 },
  { "hobgoblin",          'H',1,9,5,1,8,0 },
  { "jackal",             'J',0,12,7,1,2,0 },
  { "kobold",             'K',1,6,7,1,4,0 },
  { "leprechaun",         'L',5,15,8,1,2,0 },
  { "giant rat",          'r',0,12,7,1,3,0 },
  { "acid blob",          'a',2,3,8,0,0,0 },
  { "floating eye",       'E',2,1,9,0,0,0 },
  { "homunculus",         'h',2,6,6,1,3,0 },
  { "imp",                'i',2,6,2,1,4,0 },
  { "orc",                'O',2,9,6,1,8,0 },
  { "yellow light",       'y',3,15,0,0,0,0 },
  { "zombie",             'Z',2,6,8,1,8,0 },
  { "giant ant",          'A',3,18,3,1,6,0 },
  { "fog cloud",          'f',3,1,0,1,6,0 },
  { "nymph",              'N',6,12,9,1,2,0 },
  { "piercer",            'p',3,1,3,2,6,0 },
  { "quasit",             'Q',3,15,3,1,4,0 },
  { "quivering blob",     'q',3,1,8,1,8,0 },
  { "violet fungi",       'v',3,1,7,1,4,0 },
  { "giant beetle",       'b',4,6,4,3,4,0 },
  { "centaur",            'C',4,18,4,1,6,0 },
  { "cockatrice",         'c',4,6,6,1,3,0 },
  { "gelatinous cube",    'g',4,6,8,2,4,0 },
  { "jaguar",             'j',4,15,6,1,8,0 },
  { "killer bee",         'k',4,14,4,2,4,0 },
  { "snake",              'S',4,15,3,1,6,0 },
  { "freezing sphere",    'F',2,13,4,0,0,0 },
  { "owlbear",            'o',5,12,5,2,6,0 },
  { "rust monster",       'R',10,18,3,0,0,0 },
  { "scorpion",           's',5,15,3,1,4,0 },
  { "tengu",              't',5,13,5,1,7,0 },
  { "wraith",             'W',5,12,5,1,6,0 },
  //#ifdef NOWORM
  //  { "wumpus",             'w',8,3,2,3,6,0 },
  //#else
  { "long worm",          'w',8,3,5,1,4,0 },
  //#endif NOWORM
  { "large dog",          'd',6,15,4,2,4,0 },
  { "leocrotta",          'l',6,18,4,3,6,0 },
  { "mimic",              'M',7,3,7,3,4,0 },
  { "troll",              'T',7,12,4,2,7,0 },
  { "unicorn",            'u',8,24,5,1,10,0 },
  { "yeti",               'Y',5,15,6,1,6,0 },
  { "stalker",            'I',8,12,3,4,4,0 },
  { "umber hulk",         'U',9,6,2,2,10,0 },
  { "vampire",            'V',8,12,1,1,6,0 },
  { "xorn",               'X',8,9,-2,4,6,0 },
  { "xan",                'x',7,18,-2,2,4,0 },
  { "zruty",              'z',9,8,3,3,6,0 },
  { "chameleon",          ':',6,5,6,4,2,0 },
  { "dragon",             'D',10,9,-1,3,8,0 },
  { "ettin",              'e',10,12,3,2,8,0 },
  { "lurker above",       '\'',10,3,3,0,0,0 },
  { "nurse",              'n',11,6,0,1,3,0 },
  { "trapper",            ',',12,3,3,0,0,0 },
  { "purple worm",        'P',15,9,6,2,8,0 },
  { "demon",              '&',10,12,-4,1,4,0 },
  { "minotaur",           'm',15,15,6,4,10,0 },
  { "shopkeeper",         '@', 12, 18, 0, 4, 8, SZ_ESHK },
  // How about also:
  // ghost, wizard, mail daemon, eel; hellhound, guard; li_dog, dog, la_dog
  //struct permonst pm_ghost =
  { "ghost", ' ', 10, 3, -5, 1, 1, SZ_PLNAME },
  //struct permonst pm_wizard =
  { "wizard of Yendor", '1', 15, 12, -2, 1, 12, 0 },
  //#ifdef MAIL
  //struct permonst pm_mail_daemon = 
  { "mail daemon", '2', 100, 1, 10, 0, 0, 0 },
  //#endif MAIL
  //struct permonst pm_eel = 
  { "giant eel", ';', 15, 6, -3, 3, 6, 0 },
  //struct permonst hell_hound =
  { "hell hound", 'd', 12, 14, 2, 3, 6, 0 },
  //static struct permonst pm_guard =
  { "guard", '@', 12, 12, -1, 4, 10, SZ_EGD },
  //struct permonst li_dog =
  { "little dog", 'd',2,18,6,1,6, SZ_EDOG },
  //struct permonst dog =
  { "dog",        'd',4,16,5,1,6, SZ_EDOG },
  //struct permonst la_dog =
  { "large dog",  'd',6,15,4,2,4, SZ_EDOG }
};



static void print_permonst_stats()
{
  int szpm = sizeof(struct permonst);
  int szm = sizeof(mons);
  int i, tmp, max_len, max_len_i=0, tot_len, max_mon = szm/szpm;
  printf("sizeof(struct permons) = %d bytes\n", szpm);
  printf("There are %d items in the monsters array\n", max_mon);
  printf("(it is %d bytes)\n\n", szm);
  printf("I will find the longest string length:\n");

  max_len = tot_len = 0;
  for (i = 0 ; i < max_mon ; i++) {
    if (mons[i].mname) {
      tmp = strlen(mons[i].mname);
      tot_len += tmp + 1;
      if (tmp > max_len) {
	max_len = tmp;
	max_len_i = i;
      }
    }
  }
  printf("mname -- total is %d bytes w/ '\\0'\n", tot_len);
  printf("longest == #%d. \"%s\" (%d+1 chars)\n",
	 max_len_i, mons[max_len_i].mname, max_len);
  printf("   takes %d bytes if packed, wastes %d if not\n",
	 tot_len + 2*max_mon, (max_len+1)*max_mon - (tot_len + 2*max_mon));

}

int main()
{
  int i, max_mon, tmp;
  int names_offset;
  //  char buf[16];

  FILE *fp = fopen("gen-permonst.h", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-permonst.h\n");
    exit(-1);
  }
  print_permonst_stats(); // just for fun
  max_mon = sizeof(mons)/sizeof(struct permonst);
  fprintf(fp, "#define MAX_PERMONST      %d\n", max_mon);
  for (tmp = 0, i = 0 ; i < max_mon ; i++)  if (mons[i].mname)  tmp++;
  fprintf(fp, "#define MAX_PERMONST_NAME %d\n\n", tmp);
  fprintf(fp, "struct permonst2 {\n  short mname_offset;\n  char mlet;\n  ");
  fprintf(fp, "char mlevel;\n  char mmove;\n  char ac;\n  char damn;\n  ");
  fprintf(fp, "char damd;\n  unsigned short pxlth;\n};\n");
  fprintf(fp, "extern char mon_names[MAX_PERMONST_NAME][17];\n");
  fprintf(fp, "extern struct permonst2 mons2[MAX_PERMONST];\n");
  fclose(fp);

  fp = fopen("gen-permonst.c", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-permonst.c\n");
    exit(-1);
  }
  fprintf(fp, "#include \"gen-permonst.h\"\n"); 
  fprintf(fp, "char mon_names[MAX_PERMONST_NAME][17] = {\n"); 
  for (i = 0 ; i < max_mon ; i++) {
    if (mons[i].mname)
      fprintf(fp, "  \"%s\",\n", mons[i].mname);
  }
  fprintf(fp, "};\n\nstruct permonst2 mons2[MAX_PERMONST] = {\n"); 
  names_offset = 0;
  for (i = 0 ; i < max_mon ; i++) {
    fprintf(fp, "  { ");
    if (mons[i].mname) {
      fprintf(fp, "%3d, ", names_offset);
      names_offset += strlen(mons[i].mname) + 1;// heh.. +1 to fix my fencepost
    } else fprintf(fp, "%4d, ", -1);
    if (mons[i].mlet == '\\' || mons[i].mlet == '\'')
      fprintf(fp, "'\\%c', ", mons[i].mlet);
    else
      fprintf(fp, "'%c', ", mons[i].mlet);
    fprintf(fp, "%3d, ", mons[i].mlevel);
    fprintf(fp, "%2d, ", mons[i].mmove);
    fprintf(fp, "%2d, ", mons[i].ac);
    fprintf(fp, "%d, ", mons[i].damn);
    fprintf(fp, "%2d, ", mons[i].damd);
    fprintf(fp, "%d ", mons[i].pxlth);
    fprintf(fp, "},");
    if (mons[i].mname)
      fprintf(fp, " /* %s */", mons[i].mname);
    fprintf(fp, "\n");
  }
  fprintf(fp, "};\n\n"); 

  fclose(fp);
  return 0;
}


