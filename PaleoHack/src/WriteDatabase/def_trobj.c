#include <stdio.h>
#include <string.h>
#include "../hack.onames.h" /* for MACE etc */

#define FOOD_SYM        '%'
#define WEAPON_SYM      ')'
#define TOOL_SYM        '('
#define ARMOR_SYM       '['
#define POTION_SYM      '!'
#define SCROLL_SYM      '?'
#define WAND_SYM        '/'
#define RING_SYM        '='
//
#define        UNDEF_TYP       0
#define        UNDEF_SPE       '\177'

struct trobj {
  unsigned char trotyp;
  char trspe;
  char trolet;
  char trquan; // only needs 6 bits
  char trknown; // only needs 1 bit
};

// tourist, speleologist, fighter, knight, cave-person, wizard.  extras.

struct trobj Tourist[] = {
	{ UNDEF_TYP, 0, FOOD_SYM, 10, 1 },
	{ POT_EXTRA_HEALING, 0, POTION_SYM, 2, 0 },
	{ EXPENSIVE_CAMERA, 0, TOOL_SYM, 1, 1 },
	{ DART, 2, WEAPON_SYM, 25, 1 },	/* quan is variable */
};

struct trobj Speleologist[] = {
	{ STUDDED_LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1 },
	{ UNDEF_TYP, 0, POTION_SYM, 2, 0 },
	{ FOOD_RATION, 0, FOOD_SYM, 3, 1 },
	{ PICK_AXE, UNDEF_SPE, TOOL_SYM, 1, 0 },
	{ ICE_BOX, 0, TOOL_SYM, 1, 0 },
};

struct trobj Fighter[] = {
	{ TWO_HANDED_SWORD, 0, WEAPON_SYM, 1, 1 },
	{ RING_MAIL, 0, ARMOR_SYM, 1, 1 },
};

struct trobj Knight[] = {
	{ LONG_SWORD, 0, WEAPON_SYM, 1, 1 },
	{ SPEAR, 2, WEAPON_SYM, 1, 1 },
	{ RING_MAIL, 1, ARMOR_SYM, 1, 1 },
	{ HELMET, 0, ARMOR_SYM, 1, 1 },
	{ SHIELD, 0, ARMOR_SYM, 1, 1 },
	{ PAIR_OF_GLOVES, 0, ARMOR_SYM, 1, 1 },
};

struct trobj Cave_man[] = {
	{ MACE, 1, WEAPON_SYM, 1, 1 },
	{ BOW, 1, WEAPON_SYM, 1, 1 },
	{ ARROW, 0, WEAPON_SYM, 25, 1 },	/* quan is variable */
	{ LEATHER_ARMOR, 0, ARMOR_SYM, 1, 1 },
};

struct trobj Wizard[] = {
	{ ELVEN_CLOAK, 0, ARMOR_SYM, 1, 1 },
	{ UNDEF_TYP, UNDEF_SPE, WAND_SYM, 2, 0 },
	{ UNDEF_TYP, UNDEF_SPE, RING_SYM, 2, 0 },
	{ UNDEF_TYP, UNDEF_SPE, POTION_SYM, 2, 0 },
	{ UNDEF_TYP, UNDEF_SPE, SCROLL_SYM, 3, 0 },
};

struct trobj Tinopener[] = {
	{ CAN_OPENER, 0, TOOL_SYM, 1, 1 },
};

struct trobj * stuff[7] = {
  Tourist, Speleologist, Fighter, Knight, Cave_man, Wizard, Tinopener
};
int stuff_max[7];

// what we will print out:
typedef struct {
  unsigned char trotyp;
  char trspe;
  char trolet;
  char trquan_trknown; // 6 bits low, 1 bit high
} trobj_t;

int main()
{
  int i, j, max_obj, offset;//, tmp;
  //  char buf[16];
  FILE *fp = fopen("gen-trobj.h", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-trobj.h\n");
    exit(-1);
  }
  i = 0;
  stuff_max[i++] = sizeof(Tourist) / sizeof(struct trobj);
  stuff_max[i++] = sizeof(Speleologist) / sizeof(struct trobj);
  stuff_max[i++] = sizeof(Fighter) / sizeof(struct trobj);
  stuff_max[i++] = sizeof(Knight) / sizeof(struct trobj);
  stuff_max[i++] = sizeof(Cave_man) / sizeof(struct trobj);
  stuff_max[i++] = sizeof(Wizard) / sizeof(struct trobj);
  stuff_max[i] = sizeof(Tinopener) / sizeof(struct trobj);

  max_obj = 0;
  for (i = 0 ; i < 7 ; i++) {
    max_obj += stuff_max[i];
  }
  printf("max objs = %d\n", max_obj);
  fprintf(fp, "#define MAX_TROBJ_KINDS %d\n", 7);
  fprintf(fp, "#define MAX_TROBJS %d\n", max_obj);
  fprintf(fp, "typedef struct {\n  unsigned char trotyp;\n");
  fprintf(fp, "  char trspe;\n  char trolet;\n  char trquan_trknown;");
  fprintf(fp, "// 6 bits low, 1 bit high\n} trobj_t;\n\n");
  fprintf(fp, "extern int indices[MAX_TROBJ_KINDS+1];\n");
  fprintf(fp, "extern trobj_t trobjs[MAX_TROBJS];\n"); 
  fclose(fp);

  fp = fopen("gen-trobj.c", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-trobj.c\n");
    exit(-1);
  }
  fprintf(fp, "#include \"gen-trobj.h\"\n"); 
  /*
  fprintf(fp, "int qty[MAX_TROBJ_KINDS] = {\n  ");
  for (i = 0 ; i < 7 ; i++) {
    fprintf(fp, "%d, ", stuff_max[i]);
  }
  fprintf(fp, "\n};\n");
  */
  fprintf(fp, "int indices[MAX_TROBJ_KINDS+1] = {\n  ");
  offset = 0;
  for (i = 0 ; i < 7 ; i++) {
    fprintf(fp, "%d, ", offset);
    offset += stuff_max[i];
  }
  fprintf(fp, "%d", offset); // the end point.
  fprintf(fp, "\n};\n");
  fprintf(fp, "trobj_t trobjs[MAX_TROBJS] = {\n"); 
  for (i = 0 ; i < 7 ; i++) {
    struct trobj *ttmp = stuff[i];
    unsigned char uch;
    for (j = 0 ; j < stuff_max[i] ; j++) {
      fprintf(fp, "  { %3d, ", ttmp[j].trotyp);
      fprintf(fp, "%3d, ", ttmp[j].trspe);
      // none of these symbols need escaped.  yay.
      fprintf(fp, "'%c', ", ttmp[j].trolet);
      uch = ttmp[j].trquan; // 0x3f ..I think.
      if (ttmp[j].trknown) uch |= 0x80; // turn on sign bit
      fprintf(fp, "%3d }, \n", uch);
    }
  }  
  fprintf(fp, "};\n");

  fclose(fp);
  return 0;
}
