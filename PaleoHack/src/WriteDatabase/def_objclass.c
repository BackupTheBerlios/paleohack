#include <stdio.h>
#include <string.h>

struct objclass {
  char *oc_name;          /* actual name */
  char *oc_descr;         /* description when name unknown */
  char *oc_uname;         /* called by user */
  //unsigned char nameknown_merge; // combine 2: oc_name_known, and oc_merge
  unsigned char oc_name_known;
  unsigned char oc_merge;
  char oc_olet;
  char oc_prob;          /* probability for mkobj() */
  char oc_delay;         /* delay when using such an object */
  unsigned char oc_weight;
  char oc_oc1, oc_oc2;
  short oc_oi;
};

struct objclass2 {
  short oc_name_offset;
  short oc_descr_offset;
  unsigned char oc_uname_index;
  unsigned char nameknown_merge; // combine 2: oc_name_known, and oc_merge
  char oc_olet;
  char oc_prob;
  char oc_delay;
  unsigned char oc_weight;
  char oc_oc1;
  char oc_oc2;
  short oc_oi;
};


#define         NODIR           1
#define         IMMEDIATE       2
#define         RAY             4
#define         SPEC            1       /* +n is meaningful */


#define ILLOBJ_SYM      '\\'
#define AMULET_SYM      '"'
#define FOOD_SYM        '%'
#define WEAPON_SYM      ')'
#define TOOL_SYM        '('
#define BALL_SYM        '0'
#define CHAIN_SYM       '_'
#define ROCK_SYM        '`'
#define ARMOR_SYM       '['
#define POTION_SYM      '!'
#define SCROLL_SYM      '?'
#define WAND_SYM        '/'
#define RING_SYM        '='
#define GEM_SYM         '*'




/* objects have letter " % ) ( 0 _ ` [ ! ? / = *   */
struct objclass objects[] = {

	{ "strange object", NULL, NULL, 1, 0,
		ILLOBJ_SYM, 0, 0, 0, 0, 0, 0 },
	{ "amulet of Yendor", NULL, NULL, 1, 0,
		AMULET_SYM, 100, 0, 2, 0, 0, 0 },

#define	FOOD(name,prob,delay,weight,nutrition)	{ name, NULL, NULL, 1, 1,\
		FOOD_SYM, prob, delay, weight, 0, 0, nutrition }

/* dog eats foods 0-4 but prefers 1 above 0,2,3,4 */
/* food 4 can be read */
/* food 5 improves your vision */
/* food 6 makes you stronger (like Popeye) */
/* foods CORPSE up to CORPSE+52 are cadavers */

	FOOD("food ration", 	50, 5, 4, 800),
	FOOD("tripe ration",	20, 1, 2, 200),
	FOOD("pancake",		3, 1, 1, 200),
	FOOD("dead lizard",	3, 0, 1, 40),
	FOOD("fortune cookie",	7, 0, 1, 40),
	FOOD("carrot",		2, 0, 1, 50),
	FOOD("tin",		7, 0, 1, 0),
	FOOD("orange",		1, 0, 1, 80),
	FOOD("apple",		1, 0, 1, 50),
	FOOD("pear",		1, 0, 1, 50),
	FOOD("melon",		1, 0, 1, 100),
	FOOD("banana",		1, 0, 1, 80),
	FOOD("candy bar",	1, 0, 1, 100),
	FOOD("egg",		1, 0, 1, 80),
	FOOD("clove of garlic",	1, 0, 1, 40),
	FOOD("lump of royal jelly", 0, 0, 1, 200),

	FOOD("dead human",	0, 4, 40, 400),
	FOOD("dead giant ant",	0, 1, 3, 30),
	FOOD("dead giant bat",	0, 1, 3, 30),
	FOOD("dead centaur",	0, 5, 50, 500),
	FOOD("dead dragon",	0, 15, 150, 1500),
	FOOD("dead floating eye",	0, 1, 1, 10),
	FOOD("dead freezing sphere",	0, 1, 1, 10),
	FOOD("dead gnome",	0, 1, 10, 100),
	FOOD("dead hobgoblin",	0, 2, 20, 200),
	FOOD("dead stalker",	0, 4, 40, 400),
	FOOD("dead jackal",	0, 1, 10, 100),
	FOOD("dead kobold",	0, 1, 10, 100),
	FOOD("dead leprechaun",	0, 4, 40, 400),
	FOOD("dead mimic",	0, 4, 40, 400),
	FOOD("dead nymph",	0, 4, 40, 400),
	FOOD("dead orc",	0, 2, 20, 200),
	FOOD("dead purple worm",	0, 7, 70, 700),
	FOOD("dead quasit",	0, 2, 20, 200),
	FOOD("dead rust monster",	0, 5, 50, 500),
	FOOD("dead snake",	0, 1, 10, 100),
	FOOD("dead troll",	0, 4, 40, 400),
	FOOD("dead umber hulk",	0, 5, 50, 500),
	FOOD("dead vampire",	0, 4, 40, 400),
	FOOD("dead wraith",	0, 1, 1, 10),
	FOOD("dead xorn",	0, 7, 70, 700),
	FOOD("dead yeti",	0, 7, 70, 700),
	FOOD("dead zombie",	0, 1, 3, 30),
	FOOD("dead acid blob",	0, 1, 3, 30),
	FOOD("dead giant beetle",	0, 1, 1, 10),
	FOOD("dead cockatrice",	0, 1, 3, 30),
	FOOD("dead dog",	0, 2, 20, 200),
	FOOD("dead ettin",	0, 1, 3, 30),
	FOOD("dead fog cloud",	0, 1, 1, 10),
	FOOD("dead gelatinous cube",	0, 1, 10, 100),
	FOOD("dead homunculus",	0, 2, 20, 200),
	FOOD("dead imp",	0, 1, 1, 10),
	FOOD("dead jaguar",	0, 3, 30, 300),
	FOOD("dead killer bee",	0, 1, 1, 10),
	FOOD("dead leocrotta",	0, 5, 50, 500),
	FOOD("dead minotaur",	0, 7, 70, 700),
	FOOD("dead nurse",	0, 4, 40, 400),
	FOOD("dead owlbear",	0, 7, 70, 700),
	FOOD("dead piercer",	0, 2, 20, 200),
	FOOD("dead quivering blob",	0, 1, 10, 100),
	FOOD("dead giant rat",	0, 1, 3, 30),
	FOOD("dead giant scorpion",	0, 1, 10, 100),
	FOOD("dead tengu",	0, 3, 30, 300),
	FOOD("dead unicorn",	0, 3, 30, 300),
	FOOD("dead violet fungi",	0, 1, 10, 100),
	FOOD("dead long worm",	0, 5, 50, 500),
/* %% wt of long worm should be proportional to its length */
	FOOD("dead xan",	0, 3, 30, 300),
	FOOD("dead yellow light",	0, 1, 1, 10),
	FOOD("dead zruty",	0, 6, 60, 600),

/* weapons ... - ROCK come several at a time */
/* weapons ... - (ROCK-1) are shot using idem+(BOW-ARROW) */
/* weapons AXE, SWORD, THSWORD are good for worm-cutting */
/* weapons (PICK-)AXE, DAGGER, CRYSKNIFE are good for tin-opening */
#define WEAPON(name,prob,wt,ldam,sdam)	{ name, NULL, NULL, 1, 0 /*%%*/,\
		WEAPON_SYM, prob, 0, wt, ldam, sdam, 0 }

	WEAPON("arrow",		7, 0, 6, 6),
	WEAPON("sling bullet",	7, 0, 4, 6),
	WEAPON("crossbow bolt",	7, 0, 4, 6),
	WEAPON("dart",		7, 0, 3, 2),
	WEAPON("rock",		6, 1, 3, 3),
	WEAPON("boomerang",	2, 3, 9, 9),
	WEAPON("mace",		9, 3, 6, 7),
	WEAPON("axe",		6, 3, 6, 4),
	WEAPON("flail",		6, 3, 6, 5),
	WEAPON("long sword",	8, 3, 8, 12),
	WEAPON("two handed sword",	6, 4, 12, 6),
	WEAPON("dagger",	6, 3, 4, 3),
	WEAPON("worm tooth",	0, 4, 2, 2),
	WEAPON("crysknife",	0, 3, 10, 10),
	WEAPON("spear",		6, 3, 6, 8),
	WEAPON("bow",		6, 3, 4, 6),
	WEAPON("sling",		5, 3, 6, 6),
	WEAPON("crossbow",	6, 3, 4, 6),

	{ "whistle", "whistle", NULL, 0, 0,
		TOOL_SYM, 90, 0, 2, 0, 0, 0 },
	{ "magic whistle", "whistle", NULL, 0, 0,
		TOOL_SYM, 10, 0, 2, 0, 0, 0 },
	{ "expensive camera", NULL, NULL, 1, 1,
		TOOL_SYM, 0, 0, 3, 0, 0, 0 },
	{ "ice box", "large box", NULL, 0, 0,
		TOOL_SYM, 0, 0, 40, 0, 0, 0 },
	{ "pick-axe", NULL, NULL, 1, 1,
		TOOL_SYM, 0, 0, 5, 6, 3, 0 },
	{ "can opener", NULL, NULL, 1, 1,
		TOOL_SYM, 0, 0, 1, 0, 0, 0 },
	{ "heavy iron ball", NULL, NULL, 1, 0,
		BALL_SYM, 100, 0, 20, 0, 0, 0 },
	{ "iron chain", NULL, NULL, 1, 0,
		CHAIN_SYM, 100, 0, 20, 0, 0, 0 },
	{ "enormous rock", NULL, NULL, 1, 0,
		ROCK_SYM, 100, 0, 200 /* > MAX_CARR_CAP */, 0, 0, 0 },

#define ARMOR(name,prob,delay,ac,can)	{ name, NULL, NULL, 1, 0,\
		ARMOR_SYM, prob, delay, 8, ac, can, 0 }
	ARMOR("helmet",		 3, 1, 9, 0),
	ARMOR("plate mail",		 5, 5, 3, 2),
	ARMOR("splint mail",	 8, 5, 4, 1),
	ARMOR("banded mail",	10, 5, 4, 0),
	ARMOR("chain mail",		10, 5, 5, 1),
	ARMOR("scale mail",		10, 5, 6, 0),
	ARMOR("ring mail",		15, 5, 7, 0),
	/* the armors below do not rust */
	ARMOR("studded leather armor", 13, 3, 7, 1),
	ARMOR("leather armor",	17, 3, 8, 0),
	ARMOR("elven cloak",	 5, 0, 9, 3),
	ARMOR("shield",		 3, 0, 9, 0),
	ARMOR("pair of gloves",	 1, 1, 9, 0),

#define POTION(name,color)	{ name, color, NULL, 0, 1,\
		POTION_SYM, 0, 0, 2, 0, 0, 0 }

	POTION("restore strength",	"orange"),
	POTION("booze",		"bubbly"),
	POTION("invisibility",	"glowing"),
	POTION("fruit juice",	"smoky"),
	POTION("healing",	"pink"),
	POTION("paralysis",	"puce"),
	POTION("monster detection",	"purple"),
	POTION("object detection",	"yellow"),
	POTION("sickness",	"white"),
	POTION("confusion",	"swirly"),
	POTION("gain strength",	"purple-red"),
	POTION("speed",		"ruby"),
	POTION("blindness",	"dark green"),
	POTION("gain level",	"emerald"),
	POTION("extra healing",	"sky blue"),
	POTION("levitation",	"brown"),
	POTION(NULL,	"brilliant blue"),
	POTION(NULL,	"clear"),
	POTION(NULL,	"magenta"),
	POTION(NULL,	"ebony"),

#define SCROLL(name,text,prob) { name, text, NULL, 0, 1,\
		SCROLL_SYM, prob, 0, 3, 0, 0, 0 }
	SCROLL("mail",	"KIRJE", 0),
	SCROLL("enchant armor", "ZELGO MER", 6),
	SCROLL("destroy armor", "JUYED AWK YACC", 5),
	SCROLL("confuse monster", "NR 9", 5),
	SCROLL("scare monster", "XIXAXA XOXAXA XUXAXA", 4),
	SCROLL("blank paper", "READ ME", 3),
	SCROLL("remove curse", "PRATYAVAYAH", 6),
	SCROLL("enchant weapon", "DAIYEN FOOELS", 6),
	SCROLL("damage weapon", "HACKEM MUCHE", 5),
	SCROLL("create monster", "LEP GEX VEN ZEA", 5),
	SCROLL("taming", "PRIRUTSENIE", 1),
	SCROLL("genocide", "ELBIB YLOH",2),
	SCROLL("light", "VERR YED HORRE", 10),
	SCROLL("teleportation", "VENZAR BORGAVVE", 5),
	SCROLL("gold detection", "THARR", 4),
	SCROLL("food detection", "YUM YUM", 1),
	SCROLL("identify", "KERNOD WEL", 18),
	SCROLL("magic mapping", "ELAM EBOW", 5),
	SCROLL("amnesia", "DUAM XNAHT", 3),
	SCROLL("fire", "ANDOVA BEGARIN", 5),
	SCROLL("punishment", "VE FORBRYDERNE", 1),
	SCROLL(NULL, "VELOX NEB", 0),
	SCROLL(NULL, "FOOBIE BLETCH", 0),
	SCROLL(NULL, "TEMOV", 0),
	SCROLL(NULL, "GARVEN DEH", 0),

#define	WAND(name,metal,prob,flags)	{ name, metal, NULL, 0, 0,\
		WAND_SYM, prob, 0, 3, flags, 0, 0 }

	WAND("light",	"iridium",		10,	NODIR),
	WAND("secret door detection",	"tin",	5,	NODIR),
	WAND("create monster",	"platinum",	5,	NODIR),
	WAND("wishing",		"glass",	1,	NODIR),
	WAND("striking",	"zinc",		9,	IMMEDIATE),
	WAND("slow monster",	"balsa",	5,	IMMEDIATE),
	WAND("speed monster",	"copper",	5,	IMMEDIATE),
	WAND("undead turning",	"silver",	5,	IMMEDIATE),
	WAND("polymorph",	"brass",	5,	IMMEDIATE),
	WAND("cancellation",	"maple",	5,	IMMEDIATE),
	WAND("teleportation",	"pine",		5,	IMMEDIATE),
	WAND("make invisible",	"marble",	9,	IMMEDIATE),
	WAND("digging",		"iron",		5,	RAY),
	WAND("magic missile",	"aluminium",	10,	RAY),
	WAND("fire",	"steel",	5,	RAY),
	WAND("sleep",	"curved",	5,	RAY),
	WAND("cold",	"short",	5,	RAY),
	WAND("death",	"long",		1,	RAY),
	WAND(NULL,	"oak",		0,	0),
	WAND(NULL,	"ebony",	0,	0),
	WAND(NULL,	"runed",	0,	0),

#define	RING(name,stone,spec)	{ name, stone, NULL, 0, 0,\
		RING_SYM, 0, 0, 1, spec, 0, 0 }

	RING("adornment",	"engagement",	0),
	RING("teleportation",	"wooden",	0),
	RING("regeneration",	"black onyx",	0),
	RING("searching",	"topaz",	0),
	RING("see invisible",	"pearl",	0),
	RING("stealth",		"sapphire",	0),
	RING("levitation",	"moonstone",	0),
	RING("poison resistance", "agate",	0),
	RING("aggravate monster", "tiger eye",	0),
	RING("hunger",		"shining",	0),
	RING("fire resistance",	"gold",		0),
	RING("cold resistance",	"copper",	0),
	RING("protection from shape changers", "diamond", 0),
	RING("conflict",	"jade",		0),
	RING("gain strength",	"ruby",		SPEC),
	RING("increase damage",	"silver",	SPEC),
	RING("protection",	"granite",	SPEC),
	RING("warning",		"wire",		0),
	RING("teleport control", "iron",	0),
	RING(NULL,		"ivory",	0),
	RING(NULL,		"blackened",	0),

/* gems ************************************************************/
#define	GEM(name,color,prob,gval)	{ name, color, NULL, 0, 1,\
		GEM_SYM, prob, 0, 1, 0, 0, gval }
	GEM("diamond", "blue", 1, 4000),
	GEM("ruby", "red", 1, 3500),
	GEM("sapphire", "blue", 1, 3000),
	GEM("emerald", "green", 1, 2500),
	GEM("turquoise", "green", 1, 2000),
	GEM("aquamarine", "blue", 1, 1500),
	GEM("tourmaline", "green", 1, 1000),
	GEM("topaz", "yellow", 1, 900),
	GEM("opal", "yellow", 1, 800),
	GEM("garnet", "dark", 1, 700),
	GEM("amethyst", "violet", 2, 650),
	GEM("agate", "green", 2, 600),
	GEM("onyx", "white", 2, 550),
	GEM("jasper", "yellowish brown", 2, 500),
	GEM("jade", "green", 2, 450),
	GEM("worthless piece of blue glass", "blue", 20, 0),
	GEM("worthless piece of red glass", "red", 20, 0),
	GEM("worthless piece of yellow glass", "yellow", 20, 0),
	GEM("worthless piece of green glass", "green", 20, 0),
	{ NULL, NULL, NULL, 0, 0, ILLOBJ_SYM, 0, 0, 0, 0, 0, 0 }
};


static void print_objclass_stats()
{
  int i, max_len1, max_len2, tmp, max_len1_i, max_len2_i;
  int tot_len1, tot_len2;
  int szoc = sizeof(struct objclass);
  int szo = sizeof(objects);
  int max_obj = szo/szoc;
  printf("sizeof(struct objclass) = %d bytes\n", szoc);
  printf("There are %d items in the objects array\n", max_obj);
  printf("(it is %d bytes)\n\n", szo);
  printf("I will find the longest string length:\n");
  max_len1 = max_len2 = tot_len1 = tot_len2 = 0;
  max_len1_i = max_len2_i = 0;
  for (i = 0 ; i < max_obj-1 ; i++) {
    if (objects[i].oc_name) {
      //printf("%s\n", objects[i].oc_name);
      //printf("%d ", i);
      tmp = strlen(objects[i].oc_name);
      tot_len1 += tmp + 1;
      if (tmp > max_len1) {
	max_len1 = tmp;
	max_len1_i = i;
      }
    }
    if (objects[i].oc_descr) {
      tmp = strlen(objects[i].oc_descr);
      tot_len2 += tmp + 1;
      if (tmp > max_len2) {
	max_len2 = tmp;
	max_len2_i = i;
      }
    }
  }
  printf("oc_name -- total is %d bytes w/ '\\0'\n", tot_len1);
  printf("longest == #%d. \"%s\" (%d+1 chars)\n",
	 max_len1_i, objects[max_len1_i].oc_name, max_len1);
  printf("   takes %d bytes if packed, wastes %d if not\n",
	 tot_len1 + 2*max_obj, (max_len1+1)*max_obj - (tot_len1 + 2*max_obj));
  printf("oc_descr -- total is %d bytes w/ '\\0'\n", tot_len2);
  printf("longest == #%d. \"%s\" (%d+1 chars)\n",
	 max_len2_i, objects[max_len2_i].oc_descr, max_len2);
  printf("   takes %d bytes if packed, wastes %d if not\n",
	 tot_len2 + 2*max_obj, (max_len2+1)*max_obj - (tot_len2 + 2*max_obj));
}


// This used to live in hack's o_init.c
// But I object to people writing to the objects array unnecessarily.
void adjust_oc_prob(int max_obj)
{
  int first, last, sum, j;
  first = 0;
  // Go through the objects letter-by-letter.
  // "First" will be the start of a run of same-letter, and "last" will
  // be right after the end of that run.
  for (first = 0 ; first < max_obj ; ) {
    char c = objects[first].oc_olet;
    // Set 'last' to the first object with a _different_ olet (or a NULL name).
    for (last = first+1 ; last < max_obj ; last++)
      if (objects[last].oc_olet != c || objects[last].oc_name == NULL)
	break;
    printf("Checking prob of %c (%d)", c, last - first);
    // Make sure that the probabilities for this run add up to 100!
    // If not... assign each an equal-probability (the "+ j" part is to make
    // sure that it adds up to 100.. consider, 100/3 + 101/3 + 102/3.)
    do {
      sum = 0;
      for (j = first; j < last; j++)
	sum += objects[j].oc_prob;
      if (sum == 0) { // hm, probabilities haven't been assigned yet..
	printf("    (Adjusting probability)");
	for (j = first; j < last; j++)
	  objects[j].oc_prob = (100+j-first)/(last-first);
      }
    } while (sum == 0);
    printf("\n");

    if (sum != 100) {
      fprintf(stderr, "Error: %c probabilities out of whack\n", c);
      exit(5);
    }
    // ok actually we need to skip some stuff before finding the next run:
    // IF this is a run of things that have a Description,
    // skip anything that is still the SAME letter, but had NO name.
    // (and is not a '('... but no '('s are lacking names anyway.)
    if (objects[first].oc_descr != NULL && c != TOOL_SYM)
      while (last < max_obj && objects[last].oc_olet == c) {
	printf("    skip %d %s\n", last,
	       (objects[last].oc_descr) ? objects[last].oc_descr : "---");
	last++;
      }
    first = last;
  }
}



// there are 217 items, oc_name longest is 31+1 chars, oc_descr is 20+1 chars.
int main()
{
  int i, max_obj, tmp;
  int names_offset, descrs_offset;
  char buf[16];

  FILE *fp = fopen("gen-objclass.h", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-objclass.h\n");
    exit(-1);
  }
  print_objclass_stats(); // just for the heck of it.
  max_obj = sizeof(objects)/sizeof(struct objclass);
  adjust_oc_prob(max_obj); // ha!  save o_init.c some work.
  fprintf(fp, "#define MAX_OBJCLASS       %d\n", max_obj);
  for (tmp = 0, i = 0 ; i < max_obj ; i++)  if (objects[i].oc_name)  tmp++;
  fprintf(fp, "#define MAX_OBJCLASS_NAME  %d\n", tmp);
  for (tmp = 0, i = 0 ; i < max_obj ; i++)  if (objects[i].oc_descr)  tmp++;
  fprintf(fp, "#define MAX_OBJCLASS_DESCR %d\n\n", tmp);
  fprintf(fp, "struct objclass2 {\n  short oc_name_offset;\n  ");
  fprintf(fp, "short oc_descr_offset;\n  unsigned char oc_uname_index;\n  ");
  fprintf(fp, "unsigned char nameknown_merge;\n  char oc_olet;\n  ");
  fprintf(fp, "char oc_prob;\n  char oc_delay;\n  unsigned char oc_weight;\n");
  fprintf(fp, "  char oc_oc1;\n  char oc_oc2;\n  short oc_oi;\n};\n\n");
  fprintf(fp, "extern char oc_names[MAX_OBJCLASS_NAME][32];\n");
  fprintf(fp, "extern char oc_descrs[MAX_OBJCLASS_DESCR][21];\n");
  fprintf(fp, "extern struct objclass2 objects2[MAX_OBJCLASS];\n");
  fclose(fp);

  fp = fopen("gen-objclass.c", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-objclass.c\n");
    exit(-1);
  }

  fprintf(fp, "#include \"gen-objclass.h\"\n"); 
  fprintf(fp, "char oc_names[MAX_OBJCLASS_NAME][32] = {\n"); 
  for (i = 0 ; i < max_obj ; i++) {
    if (objects[i].oc_name)
      fprintf(fp, "  \"%s\",\n", objects[i].oc_name);
  }
  fprintf(fp, "};\nchar oc_descrs[MAX_OBJCLASS_DESCR][21] = {\n"); 
  for (i = 0 ; i < max_obj ; i++) {
    if (objects[i].oc_descr)
      fprintf(fp, "  \"%s\",\n", objects[i].oc_descr);
  }
  fprintf(fp, "};\n\nstruct objclass2 objects2[MAX_OBJCLASS] = {\n"); 
  fprintf(fp, "  // name desc u  km  c   prob delay wt  oc1 oc2  oci\n");
  names_offset = descrs_offset = 0;
  for (i = 0 ; i < max_obj ; i++) {
    fprintf(fp, "  { ");
    if (objects[i].oc_name) {
      fprintf(fp, "%4d, ", names_offset);
      names_offset += strlen(objects[i].oc_name) + 1;
    } else fprintf(fp, "%4d, ", -1);
    if (objects[i].oc_descr) {
      fprintf(fp, "%3d, ", descrs_offset);
      descrs_offset += strlen(objects[i].oc_descr) + 1;
    } else fprintf(fp, "%3d, ", -1);
    fprintf(fp, "0, "); // whether oc_uname exists.
    // oc_uname must manage its own lookup table, in a record somewhere.
    // (there won't be more than 217 so this is just an index into that array.)
    fprintf(fp, "%x, ", objects[i].oc_name_known*2 + objects[i].oc_merge);
    if (objects[i].oc_olet == '\\' || objects[i].oc_olet == '\'')
      fprintf(fp, "'\\%c', ", objects[i].oc_olet);
    else
      fprintf(fp, "'%c', ", objects[i].oc_olet);
    fprintf(fp, "%3d, ", objects[i].oc_prob);
    fprintf(fp, "%2d, ", objects[i].oc_delay);
    fprintf(fp, "%3d, ", objects[i].oc_weight);
    fprintf(fp, "%2d, ", objects[i].oc_oc1);
    fprintf(fp, "%2d, ", objects[i].oc_oc2);
    fprintf(fp, "%4d ", objects[i].oc_oi);
    fprintf(fp, "},");
    if (objects[i].oc_name) {
      strncpy(buf, objects[i].oc_name, 15);
      buf[15] = '\0';
      fprintf(fp, " /* %s */", buf);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "};\n\n"); 
  fclose(fp);
  return 0;
}
