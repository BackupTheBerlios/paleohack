#include <stdio.h>
#include <string.h>
#include "../constant.h"

// 30 names
char *shkliquors[] = {
  /* Ukraine */
  "Njezjin", "Tsjernigof", "Gomel", "Ossipewsk", "Gorlowka",
  /* N. Russia */
  "Konosja", "Weliki Oestjoeg", "Syktywkar", "Sablja",
  "Narodnaja", "Kyzyl",
  /* Silezie */
  "Walbrzych", "Swidnica", "Klodzko", "Raciborz", "Gliwice",
  "Brzeg", "Krnov", "Hradec Kralove",
  /* Schweiz */
  "Leuk", "Brig", "Brienz", "Thun", "Sarnen", "Burglen", "Elm",
  "Flims", "Vals", "Schuls", "Zum Loch",
  0
};

// 30 names
char *shkbooks[] = {
  /* Eire */
  "Skibbereen", "Kanturk", "Rath Luirc", "Ennistymon", "Lahinch",
  "Loughrea", "Croagh", "Maumakeogh", "Ballyjamesduff",
  "Kinnegad", "Lugnaquillia", "Enniscorthy", "Gweebarra",
  "Kittamagh", "Nenagh", "Sneem", "Ballingeary", "Kilgarvan",
  "Cahersiveen", "Glenbeigh", "Kilmihil", "Kiltamagh",
  "Droichead Atha", "Inniscrone", "Clonegal", "Lisnaskea",
  "Culdaff", "Dunfanaghy", "Inishbofin", "Kesh",
  0
};

// 30 names
char *shkarmors[] = {
  /* Turquie */
  "Demirci", "Kalecik", "Boyabai", "Yildizeli", "Gaziantep",
  "Siirt", "Akhalataki", "Tirebolu", "Aksaray", "Ermenak",
  "Iskenderun", "Kadirli", "Siverek", "Pervari", "Malasgirt",
  "Bayburt", "Ayancik", "Zonguldak", "Balya", "Tefenni",
  "Artvin", "Kars", "Makharadze", "Malazgirt", "Midyat",
  "Birecik", "Kirikkale", "Alaca", "Polatli", "Nallihan",
  0
};

// 31 names
char *shkwands[] = {
  /* Wales */
  "Yr Wyddgrug", "Trallwng", "Mallwyd", "Pontarfynach",
  "Rhaeader", "Llandrindod", "Llanfair-ym-muallt",
  "Y-Fenni", "Measteg", "Rhydaman", "Beddgelert",
  "Curig", "Llanrwst", "Llanerchymedd", "Caergybi",
  /* Scotland */
  "Nairn", "Turriff", "Inverurie", "Braemar", "Lochnagar",
  "Kerloch", "Beinn a Ghlo", "Drumnadrochit", "Morven",
  "Uist", "Storr", "Sgurr na Ciche", "Cannich", "Gairloch",
  "Kyleakin", "Dunvegan",
  0
};

// 32 names
char *shkrings[] = {
  /* Hollandse familienamen */
  "Feyfer", "Flugi", "Gheel", "Havic", "Haynin", "Hoboken",
  "Imbyze", "Juyn", "Kinsky", "Massis", "Matray", "Moy",
  "Olycan", "Sadelin", "Svaving", "Tapper", "Terwen", "Wirix",
  "Ypey",
  /* Skandinaviske navne */
  "Rastegaisa", "Varjag Njarga", "Kautekeino", "Abisko",
  "Enontekis", "Rovaniemi", "Avasaksa", "Haparanda",
  "Lulea", "Gellivare", "Oeloe", "Kajaani", "Fauske",
  0
};

// 32 names
char *shkfoods[] = {
  /* Indonesia */
  "Djasinga", "Tjibarusa", "Tjiwidej", "Pengalengan",
  "Bandjar", "Parbalingga", "Bojolali", "Sarangan",
  "Ngebel", "Djombang", "Ardjawinangun", "Berbek",
  "Papar", "Baliga", "Tjisolok", "Siboga", "Banjoewangi",
  "Trenggalek", "Karangkobar", "Njalindoeng", "Pasawahan",
  "Pameunpeuk", "Patjitan", "Kediri", "Pemboeang", "Tringanoe",
  "Makin", "Tipor", "Semai", "Berhala", "Tegal", "Samoe",
  0
};

// 31 names
char *shkweapons[] = {
  /* Perigord */
  "Voulgezac", "Rouffiac", "Lerignac", "Touverac", "Guizengeard",
  "Melac", "Neuvicq", "Vanzac", "Picq", "Urignac", "Corignac",
  "Fleac", "Lonzac", "Vergt", "Queyssac", "Liorac", "Echourgnac",
  "Cazelon", "Eypau", "Carignan", "Monbazillac", "Jonzac",
  "Pons", "Jumilhac", "Fenouilledes", "Laguiolet", "Saujon",
  "Eymoutiers", "Eygurande", "Eauze", "Labouheyre",
  0
};

// 30 names
char *shkgeneral[] = {
  /* Suriname */
  "Hebiwerie", "Possogroenoe", "Asidonhopo", "Manlobbi",
  "Adjama", "Pakka Pakka", "Kabalebo", "Wonotobo",
  "Akalapi", "Sipaliwini",
  /* Greenland */
  "Annootok", "Upernavik", "Angmagssalik",
  /* N. Canada */
  "Aklavik", "Inuvik", "Tuktoyaktuk",
  "Chicoutimi", "Ouiatchouane", "Chibougamau",
  "Matagami", "Kipawa", "Kinojevis",
  "Abitibi", "Maganasipi",
  /* Iceland */
  "Akureyri", "Kopasker", "Budereyri", "Akranes", "Bordeyri",
  "Holmavik",
  0
};

typedef struct  {
  char x;
  char **xn;
} shk_nx_t;
shk_nx_t shk_nx[] = {
  { POTION_SYM,	shkliquors },
  { SCROLL_SYM,	shkbooks },
  { ARMOR_SYM,	shkarmors },
  { WAND_SYM,	shkwands },
  { RING_SYM,	shkrings },
  { FOOD_SYM,	shkfoods },
  { WEAPON_SYM,	shkweapons },
  { 0,		shkgeneral }
};

/*
TO DO:  Put the data in shknam.c into the DATABASE,
            and, have a function to grab a string from it.
            (basically it picks the dlevel'th name...
            if there are dlevel names available.)

I need to add the array of shopkeeper names.
Should I make it one record per store type?  nah.
First: 8 offsets that point to the data by "store type".
Then:
  For each of the 8 store types,
      1 number (N) that indicates the number of strings for this store type.
      N offsets that point to the beginnings of strings.
      1 more offset that points to the end of the last string!
      N packed strings.  No need to make them null-terminated now!!!....
      We'll be doing a StrNCopy(buf, db_rec+offset[i], offset[i+1]-offset[i]);
no make it:

1 short = number of store types   (8)
8 shorts = number of names that that store type has   (shknam_name_counts)
1 short = total number of names
n shorts = offsets to each name
finally, all the names.
*/

#define EIGHT 8
int main()
{
  int i, j, name_counts[EIGHT];
  int offset = 0;
  int total_names = 0;
  int sz = 2*sizeof(char); // that's sizeof(Short)
  char **names;
  FILE *fp;
  // done with that
  fp = fopen("gen-shknam.c", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-shknam.c\n");
    exit(-1);
  }
  // Number of names of each type
  fprintf(fp, "#include \"gen-shknam.h\"\n\n");
  offset += sz; // write SHOP_TYPES
  offset += EIGHT * sz; // write shknam_name_counts[]
  fprintf(fp, "int shknam_name_counts[SHOP_TYPES] = {\n  ");
  for (i = 0 ; i < EIGHT ; i++) {
    char **names = shk_nx[i].xn;
    int j;
    for (j = 0 ; names[j] != 0 ; j++);
    fprintf(fp, "%d, ", j);
    name_counts[i] = j;
    total_names += j;
  }
  fprintf(fp, "\n};\n");
  // Offsets for each name, within a type
  fprintf(fp, "int shknam_name_offsets[%d] = {\n", total_names+1);
  offset += (total_names+1) * sz; // write shknam_name_offsets[]
  for (i = 0 ; i < EIGHT ; i++) {
    names = shk_nx[i].xn;
    for (j = 0 ; j < name_counts[i] ; j++) {
      if (j % 10 == 0) fprintf(fp, "\n    ");
      fprintf(fp, "%d, ", offset);
      offset += strlen(names[j]);
    }
  }
  fprintf(fp, "%d\n", offset); // end of very last string.
  fprintf(fp, "};\n");
  fprintf(fp, "int shknam_record_size = %d;\n", offset);
  // Don't forget to dump the strings, too
  fprintf(fp, "char *shknam_names[%d] = {\n", total_names);
  for (i = 0 ; i < EIGHT ; i++) {
    names = shk_nx[i].xn;
    for (j = 0 ; j < name_counts[i] ; j++) {
      if (j % 5 == 0) fprintf(fp, "\n    ");
      fprintf(fp, "\"%s\", ", names[j]);
    }
  }
  fprintf(fp, "\n};\n");
  fclose(fp);
  // now header
  fp = fopen("gen-shknam.h", "w");
  if (!fp) {
    fprintf(stderr, "can't open gen-shknam.h\n");
    exit(-1);
  }
  fprintf(fp, "#define SHOP_TYPES %d\n", EIGHT);
  fprintf(fp, "#define MAX_SHK_NAMES %d\n", total_names);
  fprintf(fp, "int shknam_record_size;\n");
  fprintf(fp, "int shknam_name_counts[SHOP_TYPES];\n");
  fprintf(fp, "int shknam_name_offsets[%d];\n", total_names+1);
  fprintf(fp, "char *shknam_names[MAX_SHK_NAMES];\n");
  fclose(fp);
}
