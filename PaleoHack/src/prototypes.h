/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 *********************************************************************/

Boolean Main_Form_HandleEvent(EventPtr e);
Boolean About_Form_HandleEvent(EventPtr e) SEC_1;
Boolean SnowCrash_Form_HandleEvent(EventPtr e);
Boolean Chargen1_Form_HandleEvent(EventPtr e) SEC_1;
Boolean Chargen2_Form_HandleEvent(EventPtr e) SEC_1;
Boolean Map_Form_HandleEvent(EventPtr e) SEC_1;
Boolean Inv_Form_HandleEvent(EventPtr e) SEC_1;
Boolean InvAction_Form_HandleEvent(EventPtr e) SEC_4;
Boolean getobj_init(Char *let, Char *word, UChar action) SEC_4;
Boolean ObjType_Form_HandleEvent(EventPtr e);// SEC_5; // debugging
Boolean Sense_Form_HandleEvent(EventPtr e) SEC_1;
Boolean MsgLog_Form_HandleEvent(EventPtr e) SEC_5;
Boolean Engrave_Form_HandleEvent(EventPtr e) SEC_5;
Boolean Tombstone_Form_HandleEvent(EventPtr e) SEC_5;
void clone_for_call(obj_t *otmp) SEC_4;
void engrave_draw() SEC_4;
void tick();
void tock();
void writePrefs();


void greet_player();
void moon_player() SEC_1;
void init_player();

// <macros>
#define DIST(x1,y1,x2,y2) (((x1)-(x2))*((x1)-(x2)) + ((y1)-(y2))*((y1)-(y2)))
#define letter(c) (('@' <= (c) && (c) <= 'Z') || ('a' <= (c) && (c) <= 'z'))
#define is_alpha(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

// util.c
Boolean hit_button_if_usable(FormPtr frm, Word btn_index);
void update_field_scrollers(FormPtr frm, FieldPtr fld,
			    Word up_scroller, Word down_scroller);
#ifndef I_AM_COLOR
void page_scroll_field(FormPtr frm, FieldPtr fld, DirectionType dir);
#else /* I_AM_COLOR */
void page_scroll_field(FormPtr frm, FieldPtr fld, WinDirectionType dir);
#endif /* I_AM_COLOR */
void LeaveForm();
Char * md_malloc(Int n);
void free_me(VoidPtr ptr);  // Please free everything you "malloc"...
Int rnd(Int y);
Int rund(Int y);
Short dice(Short n, Short x);
void do_feep(Long frq, UInt dur);
void get_default_username(Char *buf, Short max_len);
Char *my_rindex(Char *s, Char c);
void wait_for_event();
Short WinDrawChars_ctr(Char *buf, Short len, Short x_mid, Short y);

// time.c
//
Short getyear() SEC_1;
void getdate(Char *pstr) SEC_1; // Note - I changed the fn type
Short phase_of_the_moon() SEC_1;
Boolean night() SEC_1;
Boolean midnight() SEC_1;


// display.c
//
void where_in_dungeon(Short scr_x, Short scr_y, Short *dun_x, Short *dun_y);
void print(Short x, Short y, Short ch); // Equivalent of "atl"
void on_scr(Short x, Short y); // results aren't being used since nscr !exists
void check_rogue_position(Boolean centered);
void refresh();
void nscr(); // get rid of this if poss? or make it and refresh call a backend
void animate_char(Short y, Short x, Char c, Boolean bold);
void tmp_at_init(Char c) SEC_5;
void tmp_at_newsymbol(Char c) SEC_5;
void tmp_at_cleanup() SEC_5;
void tmp_at(Int8 x, Int8 y);
void Tmp_at_init(Char c);
void Tmp_at_newsymbol(Char c);
void Tmp_at_cleanup();
void Tmp_at(Int8 x, Int8 y);
void swallowed();
void prme();
Boolean vism_at(Short x, Short y);
void pru();
void prl(Short x, Short y);
void newsym(Short x, Short y);
void mnewsym(Short x, Short y);
void prl1(Short x, Short y);
void nose1(Short x, Short y);
void unpobj(obj_t *obj);
void seeobjs(); // WHY is this called seeobjs?  It should be decay_corpses!
void seemons();
void pmon(monst_t *mon);
void unpmon(struct monst *mon);
Boolean toggle_itsy();
void draw_directional();
void undraw_directional();

//
void level_message(Char *buf);
void message(const Char *buf);
void alert_message(Char *buf);
Boolean message_clear(Boolean really);
void preempt_messages();
void show_all_messages();
void show_messages();
void show_a_more(Short lines_used);
void alloc_message_log();
void clear_message_log();
void print_stats(UInt which_stats);



// make_level.c -- this is ""done""
void makelevel() SEC_1; // xxx debug
Short inroom(Short x, Short y) SEC_1;
void mktrap(Short num, Short mazeflag, room_t *croom) SEC_1;
// make_maze.c -- this is ""done""
void makemaz() SEC_1;
PointType mazexy() SEC_1;


// trap.c
trap_t *maketrap(Short x, Short y, Short trap_type) SEC_1;
void deltrap(trap_t *trap) SEC_2; // was in invent.c!
void do_trap(trap_t *trap) SEC_3;
Short mon_in_trap(monst_t *mtmp) SEC_3;
void selftouch(Char *arg) SEC_2;
void float_up() SEC_2;
void float_down() SEC_2;
void tele() SEC_3;
void tele_finish(Short x, Short y, Boolean controlled) SEC_3;
Boolean dotele() SEC_3;
void placebc(Boolean attach) SEC_3;
void unplacebc() SEC_3;
void level_tele_start() SEC_3;
void level_tele(Short newlevel, Boolean controlled) SEC_3;
void drown() SEC_2; // untested

// make_obj.c
obj_t * mkobj_at(Short symbol, Short x, Short y) SEC_1;
void mksobj_at(Short objtype, Short x, Short y) SEC_1;
obj_t * mksobj(Short otyp) SEC_2;
obj_t * mkobj(Short c) SEC_1;
Short weight(obj_t *obj) SEC_1;
void mkgold(Long num, Short x, Short y) SEC_2;
// invent.c
obj_t * addinv(obj_t *obj) SEC_1;
void useup(obj_t *obj) SEC_2;
void unlink_inv(obj_t *obj) SEC_2; // was (poorly named) "freeinv"
void delobj(struct obj *obj) SEC_2; 
void unlink_obj(obj_t *obj) SEC_2; // was (poorly named) "freeobj"
void freegold(gold_t *gold) SEC_2;
trap_t *trap_at(Short x, Short y) SEC_2; // was t_at
monst_t *mon_at(Short x, Short y) SEC_2; // was m_at
obj_t * obj_at(Short x, Short y) SEC_2; // was o_at
obj_t * sobj_at(Short n, Short x, Short y) SEC_2;
Boolean carried(obj_t *obj) SEC_2;
Boolean carrying(Short type) SEC_2;
gold_t * gold_at(Short x, Short y) SEC_2; // was g_at
Short ggetobj_end(Char *olets, Boolean drop_not_identify,
		  Boolean allflag, Boolean unpaidflag);// SEC_5; // XXX
Short askchain(obj_t *objchn, Char *olets, Char *prompt, Boolean allflag, Short (*fn)(), Boolean (*ckfn)(), Short max); // XXX danger will robinson
void prinv(obj_t *obj);
void stackobj(obj_t *obj) SEC_1;
obj_t * splitobj(obj_t *obj, Short num) SEC_1; //was in "do.c"
// make_mon.c
monst_t * makemon(permonst_t *ptr, Short x, Short y) SEC_2;
PointType enexto(Int8 xx, Int8 yy) SEC_2;
Boolean goodpos(Short x, Short y) SEC_2; // (zap needs this too)
void rloc(monst_t *mtmp) SEC_2;
monst_t * mkmon_at(Char let, Short x, Short y) SEC_2;
// mon.c
void movemon() SEC_3;
void justswld(monst_t *mtmp, Char *name) SEC_3;
void youswld(monst_t *mtmp, Short dam, Short die, Char *name) SEC_3;
Boolean do_chug(monst_t *mtmp) SEC_3; // was dochug
Short m_move(monst_t *mtmp, Short after) SEC_3;
Short mfindpos(monst_t *mon, coord poss[9], // mfindpos was mfndpos
	       Short info[9], Short flag) SEC_3;
Short dist(Short x, Short y) SEC_2;
void poisoned(Char *string, Char *pname) SEC_3;
void mondead(monst_t *mtmp) SEC_2;
void replmon(monst_t *mtmp, monst_t *mtmp2) SEC_3;
void unlink_mon(struct monst *mon) SEC_2; // was (mal)named "relmon"
void monfree(monst_t *mtmp) SEC_3;
void unstuck(monst_t *mtmp) SEC_2;
void charcat(Char *s, Char c) SEC_2;
void killed(monst_t *mtmp) SEC_2;
void kludge(Char *str, Char *arg) SEC_3;
void res_cham() SEC_2; // was rescham
Boolean newcham(monst_t *mtmp, permonst_t *mdat) SEC_2;
void mnexto(monst_t *mtmp) SEC_2;
void setmangry(monst_t *mtmp) SEC_1;
Boolean canseemon(monst_t *mtmp) SEC_2;
// dog.c
void makedog() SEC_4;
void losedogs() SEC_4; 
void keepdogs() SEC_4;
void fall_down(monst_t *mtmp) SEC_4;
   // much more missing from dog.c...
Short dog_move(monst_t *mtmp, Short after) SEC_4;
Boolean tamedog(monst_t *mtmp, obj_t *obj) SEC_4;



// obj_init.c
Short letindex(Char let) SEC_1;
void init_objects() SEC_1;
Short probtype(Char c) SEC_1;
void oinit() SEC_1;
Boolean dodiscovered() SEC_2;
Boolean interesting_to_discover(Short i) SEC_2;
// make_zoo.c
void make_shop() SEC_5; // xxx debug
void make_zoo(Short zoo_type) SEC_2;
void make_swamp() SEC_2;
// engrave.c
Boolean sengr_at(Char *s, Short x, Short y) SEC_4;
void you_wipe_engr(Short cnt) SEC_4;
void wipe_engr_at(Short x, Short y, Short cnt) SEC_4;
void read_engr_at(Short x, Short y) SEC_4;
void make_engr_at(Short x, Short y, Char *s) SEC_4;
Boolean check_do_engrave() SEC_4;
Boolean do_engrave(obj_t *otmp, Char *buf, Int8 type) SEC_4;
Short save_engravings_size() SEC_5; // XXX not tested yet
Short save_engravings(VoidPtr p, Short offset) SEC_5; // XXX not tested yet
void rest_engravings(VoidPtr *p) SEC_5; // XXX not tested yet
// level.c
void mklev() SEC_1; // (trivial)  // xxx debug
void savelev(Short lev, Boolean not_bones) SEC_5;
Short saveobjchn_size(obj_t *otmp) SEC_5;  // xxx these all need testing:
Short saveobjchn(VoidPtr p, Short offset, obj_t *otmp) SEC_5;
obj_t * restobjchn(VoidPtr *p) SEC_5;  // THEN they can be sec5.
Short savemonchn_size(monst_t *mtmp) SEC_5;
Short savemonchn(VoidPtr p, Short offset, monst_t *mtmp) SEC_5;
monst_t * restmonchn(VoidPtr *p) SEC_5;
void save_tag(VoidPtr p, Short *offset, Char *tag);
void check_tag(VoidPtr *p, Char *tag);
Boolean getlev(UChar lev, Boolean not_bones) SEC_5;
// save.c
Boolean dosave() SEC_5; // xxx these need testing too.
Boolean dorecover() SEC_5;
// bones.c
void savebones() SEC_5; // xxx these need testing too.
Boolean getbones() SEC_5; // xxx debug
// end.c
void unsave() SEC_5;
void done_in_by(monst_t *mtmp) SEC_5;
void done(Char *st1) SEC_5;
Short done_postRIP_size() SEC_5;
void done_postRIP(Char *buf) SEC_5;
void draw_topten() SEC_5;
// movesee.c was hack.c
void unsee() SEC_4;
void nomul(Short nval) SEC_1;
void confdir() SEC_1; // was in cmd.c
void seeoff(Boolean mode) SEC_1;
Boolean do_move() SEC_2;
Boolean do_pickup() SEC_4;
void pickup(Boolean all) SEC_1;
void lookaround() SEC_1;
Boolean monster_nearby() SEC_2;
Boolean cansee(Short x, Short y) SEC_2;
void setsee() SEC_1;
void litroom(Boolean on) SEC_2; // untested
Short abon() SEC_2;
Short dbon() SEC_2;
void losestr(Short num) SEC_2;
void losehp(Short n, Char *knam) SEC_2; // death-check might be screwy
void losehp_m(Short n, struct monst *mtmp) SEC_2; // ditto
Long newuexp() SEC_1;
void losexp() SEC_2;
Boolean do_down() SEC_5;
Boolean do_up() SEC_5;
void goto_level(Short newlevel, Boolean at_stairs) SEC_5;
void more_experienced(Short exp, Short rexp) SEC_2;
void pluslvl() SEC_2;
Short inv_weight() SEC_1;
void set_wounded_legs(Long side, Short timex) SEC_3;
void heal_legs() SEC_2;
// wield.c
void setuwep(struct obj *obj) SEC_1; // wrapper for setworn!
Boolean do_wield(obj_t *wep) SEC_1;
void corrode_weapon() SEC_1;
Boolean chwepon(obj_t *otmp, Short amount) SEC_1;
// worn.c
void setworn(obj_t *obj, Long mask) SEC_1;
void setnotworn(obj_t *obj) SEC_1;
Boolean do_remove_armor(obj_t *otmp) SEC_1; // was doremarm
Boolean do_remove_ring(obj_t *otmp) SEC_1; // was doremring
Boolean armoroff(obj_t *otmp) SEC_1;
Boolean do_wear_armor(obj_t *otmp) SEC_1; // was doweararm
Boolean do_wear_ring(obj_t *otmp) SEC_1;
void ringoff(obj_t *obj) SEC_1;
void find_ac() SEC_1;
void slippery_fingers() SEC_1; // was "glibr()"
obj_t * some_armor() SEC_1;
void corrode_armor() SEC_3;
// obj_name.c
Boolean is_vowel(Char v) SEC_4;
Char * typename(Short otyp) SEC_4;
Short typename_len(Short otyp) SEC_4;
Char * xname(struct obj *obj) SEC_1;
Char * doname(obj_t *obj) SEC_1;
Char * aobjnam(obj_t *otmp, Char *verb) SEC_1;
Char * Doname(obj_t *obj) SEC_1;
obj_t * readobjnam(Char *bp) SEC_4;
// search.c
Short findit() SEC_2;
void do_search() SEC_1; // semi-tested
Boolean do_id_trap() SEC_4; // untested
void wakeup(monst_t *mtmp) SEC_1;
void see_mimic(monst_t *mtmp) SEC_1; // untested
// do_name.c
void do_mname(monst_t *mtmp, Char *new_name) SEC_5;
void do_name(obj_t *otmp, Char *new_name) SEC_4;
void do_call(obj_t *otmp, Char *new_name) SEC_4;
Boolean oc_has_uname(Short otype) SEC_4;
Char *oc_get_uname(Short otype) SEC_4;
Char * monnam(monst_t *mtmp) SEC_1;
void lmonnam(monst_t *mtmp, Char *buf);// SEC_1; // like xmonnam
Char * Monnam(monst_t *mtmp) SEC_1;
Char * amonnam(monst_t *mtmp, Char *adj) SEC_1;
Char * Amonnam(monst_t *mtmp, Char *adj) SEC_1;
Char * Xmonnam(monst_t *mtmp) SEC_1;
// drop.c
Short drop(obj_t *obj) SEC_5;
void dropx(obj_t *obj) SEC_5;
Boolean do_throw() SEC_5; // was dothrow
// eat.c
Boolean eat_off_floor() SEC_1;
Boolean do_eat(obj_t *otmp);
void gethungry() SEC_2;
void morehungry(Short num) SEC_2;
void lesshungry(Short num) SEC_2;               // just in eat.c and potion.c
Boolean poisonous(obj_t *otmp) SEC_2;           // just in eat.c and dog.c
// potion.c
Boolean do_drink(obj_t *otmp) SEC_2; // still missing a line at end for p_tofn
void finish_do_drink(obj_t *otmp, Boolean nothing, Boolean unkn) SEC_2;
void strange_feeling(obj_t *obj, Char *txt) SEC_2; // needs do_call + uname...
void potionhit(monst_t *mon, obj_t *obj) SEC_1;
void potionbreathe(obj_t *obj) SEC_1;
Boolean do_dip(obj_t *potion, obj_t *obj) SEC_1;
// read.c
Boolean do_read(obj_t *scroll) SEC_2;
void finish_do_scroll(obj_t *scroll, Boolean known, Boolean confused) SEC_2;
void do_genocide(Char *buf) SEC_4;
// zap.c
Boolean do_zap(obj_t *obj) SEC_3;
Boolean do_zap_helper() SEC_3;
Char exclaim(Short force) SEC_3;
void hit_message(Char *str, monst_t *mtmp, Char punct) SEC_3; // was 'hit(...)'
void miss_message(Char *str, monst_t *mtmp) SEC_3; // was 'miss(...)'
monst_t * bhit(Short ddx, Short ddy, Short range, Char sym,
	       Boolean call_fhit, obj_t *obj) SEC_3;
monst_t * boomhit(Short dx, Short dy, Boolean *caught);// SEC_3;
void buzz(Short type, Short sx, Short sy, Short dx, Short dy) SEC_3;
void fracture_rock(obj_t *obj) SEC_3;
// apply.c
Boolean do_apply(obj_t *obj) SEC_1;
void use_camera(obj_t *obj) SEC_1;
Boolean put_in_ice_box(obj_t *obj) SEC_2; // was "in_ice_box"
Short holetime() SEC_2;
void dighole() SEC_2;
Boolean use_pick_axe(obj_t *obj) SEC_1;
// track.c
void init_track() SEC_4;
void set_track() SEC_4;
coord * get_track(Short x, Short y) SEC_4;
// timeout.c
void timeout() SEC_4; // missing a few lines
// vault.c
void setgd() SEC_4; // All of vault.c is UNTESTED.
void invault() SEC_4;
void do_vault(Char *buf) SEC_4;
Short gd_move() SEC_4;
void gddead() SEC_4;
void replgd(monst_t *mtmp, monst_t *mtmp2) SEC_4;
// wizard.c
void amulet() SEC_4;
void inrange(monst_t *mtmp) SEC_4;
Boolean wiz_hit(monst_t *mtmp) SEC_4;
// fight.c
Short hitmm(monst_t *magr, monst_t *mdef) SEC_3;
void mondied(monst_t *mdef) SEC_3;
Short fightm(monst_t *mtmp) SEC_3;
Boolean thing_hit_you(Short tlev, Short dam, Char *name) SEC_3; //was thitu
Boolean hit_mon(monst_t *mon, obj_t *obj, Short thrown) SEC_3; // was hmon
Boolean attack(monst_t *mtmp) SEC_3;
// worm.c
Boolean getwn(monst_t *mtmp) SEC_3;
void initworm(monst_t *mtmp) SEC_3;
void worm_move(monst_t *mtmp) SEC_3;
void worm_nomove(monst_t *mtmp) SEC_3;
void wormdead(monst_t *mtmp) SEC_3;
void wormhit(monst_t *mtmp) SEC_3;
void wormsee(UInt tmp) SEC_3;
void pwseg(wseg_t *wtmp) SEC_3;
void cutworm(monst_t *mtmp, Short x, Short y, UChar weptyp) SEC_3;
// monhityou.c
Boolean mon_hit_you(monst_t *mtmp) SEC_3; // was mhitu
Boolean hit_you(monst_t *mtmp, Short dam) SEC_3; // was hitu
// steal.c
Long somegold() SEC_4;
void stealgold(monst_t *mtmp) SEC_4;
Boolean steal(monst_t *mtmp) SEC_4;
void mpickobj(monst_t *mtmp, obj_t *otmp) SEC_4;
Boolean stealamulet(monst_t *mtmp) SEC_4;
void release_objs(monst_t *mtmp, Boolean show) SEC_4; // was relobj
// shk.c
Char * shkname(monst_t *mtmp) SEC_4;
void shkdead(monst_t *mtmp) SEC_4;
void replshk(monst_t *mtmp, monst_t *mtmp2) SEC_4;
UInt inshop() SEC_4;
void free_obj(obj_t *obj, obj_t *merge) SEC_4; // was called obfree
Boolean dopay() SEC_4;
void paybill() SEC_4;
void addtobill(obj_t *obj) SEC_4;
void splitbill(obj_t *obj, obj_t *otmp) SEC_4;
void subfrombill(struct obj *obj) SEC_4;
Short doinvbill(Short mode) SEC_4; /* 0: deliver count 1: paged */
Boolean shkcatch(obj_t *obj) SEC_4;
Short shk_move(monst_t *shkp) SEC_4;
void shopdig(Boolean fall) SEC_4;
Boolean online(Short x, Short y) SEC_4;
Boolean follower(monst_t *mtmp) SEC_4; // also used in dog.c

// lock.c
void lock_const_recs();
void unlock_const_recs();
void lock_volatile_recs();
void unlock_volatile_recs();
void rec_uname_init();
void rec_uname_edit(Char *buf, Short i);
Char *rec_uname_get(Short i);
void rec_ocdescr_init();
void rec_ocdescr_set(Short i, Short val);




