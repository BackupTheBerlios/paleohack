/*********************************************************************
 * paleohack - Hack 1.0.3 for PalmOS.                                *
 * (C) 2001 Bridget Spitznagel and Stichting Mathematisch Centrum    *
 * Hack 1.0.3 is (C) 1985 Stichting Mathematisch Centrum, Amsterdam  *
 *********************************************************************/
#include "paleohack.h"
#include "paleohackRsc.h"

extern room_t rooms[MAX_ROOMS]; // in make_level.c .... SIGH...

Char plname[PL_NSIZ]; // "Hotsync Name Here"

#include "shk.h"
#define ESHK(mp)        ( (eshk_t *)((mp)->extra) )

#define	NOTANGRY(mon)	(mon->bitflags & M_IS_PEACEFUL)
#define	ANGRY(mon)	!NOTANGRY(mon)

/* Descriptor of current shopkeeper. Note that the bill need not be
   per-shopkeeper, since it is valid only when in a shop. */
static monst_t *shopkeeper = 0;
static bill_t *bill;
static Short shlevel = 0;	/* level of this shopkeeper */
obj_t *billobjs = NULL; /* objects on bill with bp->useup
		  * only accessed here and by save & restore */
static Long total;		/* filled by addupbill() */
static Long followmsg;	/* last time of follow message */
/*
  invariants: obj->unpaid iff onbill(obj) [unless bp->useup]
  obj->quan <= bp->bquan
*/

// should be const (but can't; compiler problems)
Char shtypes[] = {	/* 8 shoptypes: 7 specialized, 1 mixed */
  RING_SYM, WAND_SYM, WEAPON_SYM, FOOD_SYM, SCROLL_SYM,
  POTION_SYM, ARMOR_SYM, 0
};

static Char *shopnam[] = {
  "engagement ring", "walking cane", "antique weapon",
  "delicatessen", "second hand book", "liquor",
  "used armor", "assorted antiques"
};

static void setpaid() SEC_4;
static void addupbill() SEC_4;
static void findshk(Short roomno) SEC_4;
static bill_t * onbill(obj_t *obj) SEC_4;
static void pay(Long tmp, monst_t *shkp) SEC_4;
obj_t * bp_to_obj(bill_t *bp) SEC_4;
static obj_t * o_on(UInt id, obj_t *objchn) SEC_4; // was in invent.c
static Short dopayobj(bill_t *bp) SEC_4;
static Short getprice(obj_t *obj) SEC_4;
static Short realhunger() SEC_4;


/* called in do_name.c */
Char * shkname(monst_t *mtmp)
{
  return ESHK(mtmp)->shknam;
}

/* called in mon.c */
void shkdead(monst_t *mtmp)
{
  eshk_t *eshk = ESHK(mtmp);

  if (eshk->shoplevel == dlevel)
    rooms[eshk->shoproom].rtype = 0;
  if (mtmp == shopkeeper) {
    setpaid();
    shopkeeper = NULL;
    //bill = (bill_t *) -1000;	/* dump core when referenced */  // XXXXX !!!
    bill = NULL; // this seems like a better idea to me
  }
}

void replshk(monst_t *mtmp, monst_t *mtmp2)
{
  if (mtmp == shopkeeper) {
    shopkeeper = mtmp2;
    bill = ESHK(shopkeeper)->bill; //&(ESHK(shopkeeper)->bill[0]);
  }
}

/* caller has checked that shopkeeper exists */
/* either we paid or left the shop or he just died */
static void setpaid()
{
  obj_t *obj;
  monst_t *mtmp;
  for (obj = invent ; obj ; obj = obj->nobj)
    obj->bitflags &= ~O_IS_UNPAID;
  for (obj = fobj ; obj ; obj = obj->nobj)
    obj->bitflags &= ~O_IS_UNPAID;
  for (obj = fcobj ; obj ; obj = obj->nobj)
    obj->bitflags &= ~O_IS_UNPAID;
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    for (obj = mtmp->minvent ; obj ; obj = obj->nobj)
      obj->bitflags &= ~O_IS_UNPAID;
  for (mtmp = fallen_down ; mtmp ; mtmp = mtmp->nmon)
    for (obj = mtmp->minvent ; obj ; obj = obj->nobj)
      obj->bitflags &= ~O_IS_UNPAID;
  while ((obj = billobjs)) {
    billobjs = obj->nobj;
    free_me((VoidPtr) obj); // hmmmm.
  }
  ESHK(shopkeeper)->billct = 0;
}


/* puts the result in global variable "total" */
/* caller has checked that shopkeeper exists */
static void addupbill()
{
  Short ct = ESHK(shopkeeper)->billct;
  bill_t *bp = bill;
  total = 0;
  while (ct--) {
    total += bp->price * bp->bquantity;
    bp++;
  }
}


UInt inshop()
{
  Short roomno = inroom(you.ux,you.uy);

  /* Did we just leave a shop? */
  if (you.uinshop &&
      (you.uinshop != roomno + 1 || shlevel != dlevel || !shopkeeper)) {
    if (shopkeeper) {
      if (ESHK(shopkeeper)->billct) {
	if (inroom(shopkeeper->mx, shopkeeper->my) 
	    == you.uinshop - 1)	/* ab@unido */
	  message("Somehow you escaped the shop without paying!");
	addupbill();
	StrPrintF(ScratchBuffer,"You stole for a total worth of %ld zorkmids.",
		  total);
	message(ScratchBuffer);
	ESHK(shopkeeper)->robbed += total;
	setpaid();
	if ((rooms[ESHK(shopkeeper)->shoproom].rtype == GENERAL)
	    == (rund(3) == 0))
	  ESHK(shopkeeper)->following = true;
      }
      shopkeeper = NULL;
      shlevel = 0;
    }
    you.uinshop = 0;
  }

  /* Did we just enter a zoo of some kind? */
  if (roomno >= 0) {
    Short rt = rooms[roomno].rtype;
    monst_t *mtmp;
    if (rt == ZOO) {
      message("Welcome to David's treasure zoo!");
    } else
      if (rt == SWAMP) {
	message("It looks rather muddy down here.");
      } else
	if (rt == MORGUE) {
	  if (midnight())
	    message("Go away! Go away!");
	  else
	    message("You get an uncanny feeling ...");
	} else
	  rt = 0;
    if (rt != 0) {
      rooms[roomno].rtype = 0;
      for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
	if (rt != ZOO || !rund(3))
	  mtmp->bitflags &= ~M_IS_ASLEEP;
    }
  }

  /* Did we just enter a shop? */
  if (roomno >= 0 && rooms[roomno].rtype >= 8) {
    if (shlevel != dlevel || !shopkeeper
	|| ESHK(shopkeeper)->shoproom != roomno)
      findshk(roomno);
    if (!shopkeeper) {
      rooms[roomno].rtype = 0;
      you.uinshop = 0;
    } else if (!you.uinshop) {
      if (!ESHK(shopkeeper)->visitct ||
	  StrNCompare(ESHK(shopkeeper)->customer, plname, PL_NSIZ)) {

	/* He seems to be new here */
	ESHK(shopkeeper)->visitct = 0;
	ESHK(shopkeeper)->following = false;
	StrNCopy(ESHK(shopkeeper)->customer, plname, PL_NSIZ);
	shopkeeper->bitflags |= M_IS_PEACEFUL; // NOTANGRY(shopkeeper) = 1;
      }
      if (!ESHK(shopkeeper)->following) {
	Boolean box, pick;

	StrPrintF(ScratchBuffer, "Hello %s! Welcome%sto %s's %s shop!",
		  plname,
		  ESHK(shopkeeper)->visitct++ ? " again " : " ",
		  shkname(shopkeeper),
		  shopnam[rooms[ESHK(shopkeeper)->shoproom].rtype - 8] );
	message(ScratchBuffer);
	box = carrying(ICE_BOX);
	pick = carrying(PICK_AXE);
	if (box || pick) {
	  if (do_chug(shopkeeper)) {
	    you.uinshop = 0;	/* he died moving */
	    return 0;
	  }
	  StrPrintF(ScratchBuffer, "Will you please leave your %s outside?",
		    (box && pick) ? "box and pick-axe" :
		    box ? "box" : "pick-axe");
	  message(ScratchBuffer);
	}
      }
      you.uinshop = roomno + 1; // (0 is "not in shop")
    }
  }
  return you.uinshop;
}


static void findshk(Short roomno)
{
  monst_t *mtmp;
  for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
    if ((mtmp->bitflags & M_IS_SHOPKEEPER) && ESHK(mtmp)->shoproom == roomno
	&& ESHK(mtmp)->shoplevel == dlevel) {
      shopkeeper = mtmp;
      bill = ESHK(shopkeeper)->bill;
      shlevel = dlevel;
      if (ANGRY(shopkeeper) &&
	  StrNCompare(ESHK(shopkeeper)->customer, plname, PL_NSIZ))
	shopkeeper->bitflags |= M_IS_PEACEFUL; // NOTANGRY(shopkeeper) = 1;
      /* billobjs = 0; -- this is wrong if we save in a shop */
      /* (and it is harmless to have too many things in billobjs) */
      return;
    }
  shopkeeper = NULL;
  shlevel = 0;
  // bill = (struct bill_x *) -1000;	/* dump core when referenced */
  bill = NULL;
}

static bill_t * onbill(obj_t *obj)
{
  bill_t *bp;
  if (!shopkeeper) return NULL;
  for (bp = bill ; bp < &bill[ESHK(shopkeeper)->billct] ; bp++)
    if (bp->bo_id == obj->o_id) {
      if (!(obj->bitflags & O_IS_UNPAID))
	message("BUG: onbill: paid obj on bill?");
      return bp;
    }
  if (obj->bitflags & O_IS_UNPAID)
    message("BUG: onbill: unpaid obj not on bill?");
  return NULL;
}


/* called with two args on merge */
void free_obj(obj_t *obj, obj_t *merge) // was called obfree
{
  bill_t *bp = onbill(obj);
  bill_t *bpm;
  if (bp) {
    if (!merge) {
      bp->useup = true;
      obj->bitflags &= ~O_IS_UNPAID;	/* only for doinvbill */
      obj->nobj = billobjs;
      billobjs = obj;
      return;
    }
    bpm = onbill(merge);
    if (!bpm) {
      /* this used to be a rename */
      message("BUG: obfree: not on bill??");
      return;
    } else {
      /* this was a merger */
      bpm->bquantity += bp->bquantity;
      ESHK(shopkeeper)->billct--;
      *bp = bill[ESHK(shopkeeper)->billct];
    }
  }
  free_me((VoidPtr) obj);
}


static void pay(Long tmp, monst_t *shkp)
{
  Long robbed = ESHK(shkp)->robbed;

  you.ugold -= tmp;
  shkp->mgold += tmp;
  flags.botl |= BOTL_GOLD;
  if (robbed) {
    robbed -= tmp;
    if (robbed < 0) robbed = 0;
    ESHK(shkp)->robbed = robbed;
  }
}

extern Short multi; // living in movesee.c right now..
// What does the return value indicate??
Boolean dopay()
{
  Long ltmp;
  bill_t *bp;
  monst_t *shkp;
  Short pass, tmp;

  multi = 0;
  inshop();
  for (shkp = fmon ; shkp ; shkp = shkp->nmon)
    if ((shkp->bitflags & M_IS_SHOPKEEPER) && dist(shkp->mx,shkp->my) < 3)
      break;
  if (!shkp && you.uinshop &&
      inroom(shopkeeper->mx,shopkeeper->my) == ESHK(shopkeeper)->shoproom)
    shkp = shopkeeper;

  if (!shkp) {
    message("There is nobody here to receive your payment.");
    return false;
  }
  ltmp = ESHK(shkp)->robbed;
  if (shkp != shopkeeper && NOTANGRY(shkp)) {
    if (!ltmp) {
      StrPrintF(ScratchBuffer, "You do not owe %s anything.", monnam(shkp));
      message(ScratchBuffer);
    } else
      if (!you.ugold) {
	message("You have no money.");
      } else {
	Long ugold = you.ugold;

	if (you.ugold > ltmp) {
	  StrPrintF(ScratchBuffer,
		    "You give %s the %ld gold pieces he asked for.",
		    monnam(shkp), ltmp);
	  message(ScratchBuffer);
	  pay(ltmp, shkp);
	} else {
	  StrPrintF(ScratchBuffer, "You give %s all your gold.", monnam(shkp));
	  message(ScratchBuffer);
	  pay(you.ugold, shkp);
	}
	if (ugold < ltmp/2) {
	  message("Unfortunately, he doesn't look satisfied.");
	} else {
	  ESHK(shkp)->robbed = 0;
	  ESHK(shkp)->following = false;
	  if (ESHK(shkp)->shoplevel != dlevel) {
	    /* For convenience's sake, let him disappear */
	    shkp->minvent = NULL;	/* %% */ // xxx leak?
	    shkp->mgold = 0;
	    mondead(shkp);
	  }
	}
      }
    return true;
  }
		
  if (!ESHK(shkp)->billct) {
    StrPrintF(ScratchBuffer, "You do not owe %s anything.", monnam(shkp));
    message(ScratchBuffer);
    if (!you.ugold) {
      message("Moreover, you have no money.");
      return true;
    }
    if (ESHK(shkp)->robbed) {
      message("But since his shop has been robbed recently,");
      StrPrintF(ScratchBuffer, "you%srepay %s's expenses.",
		(you.ugold < ESHK(shkp)->robbed) ? " partially " : " ",
		monnam(shkp));
      message(ScratchBuffer);
      pay(min(you.ugold, ESHK(shkp)->robbed), shkp);
      ESHK(shkp)->robbed = 0;
      return true;
    }
    if (ANGRY(shkp)) {
      StrPrintF(ScratchBuffer, "But in order to appease %s,",
		amonnam(shkp, "angry"));
      message(ScratchBuffer);
      if (you.ugold >= 1000) {
	ltmp = 1000;
	message(" you give him 1000 gold pieces.");
      } else {
	ltmp = you.ugold;
	message(" you give him all your money.");
      }
      pay(ltmp, shkp);
      if (StrNCompare(ESHK(shkp)->customer, plname, PL_NSIZ)
	  || rund(3)){
	StrPrintF(ScratchBuffer, "%s calms down.", Monnam(shkp));
	message(ScratchBuffer);
	shkp->bitflags |= M_IS_PEACEFUL; // NOTANGRY(shopkeeper) = 1;
      } else {
	StrPrintF(ScratchBuffer, "%s is as angry as ever.",
		  Monnam(shkp));
	message(ScratchBuffer);
      }
    }
    return true;
  }
  if (shkp != shopkeeper) {
    message("BUG: dopay: not to shopkeeper?");
    if (shopkeeper) setpaid();
    return false;
  }
  for (pass = 0 ; pass <= 1 ; pass++) {
    tmp = 0;
    while (tmp < ESHK(shopkeeper)->billct) {
      bp = &bill[tmp];
      if (!pass && !bp->useup) {
	tmp++;
	continue;
      }
      if (!dopayobj(bp)) return true;
      bill[tmp] = bill[--ESHK(shopkeeper)->billct];
    }
  }
  StrPrintF(ScratchBuffer, "Thank you for shopping in %s's %s store!",
	    shkname(shopkeeper),
	    shopnam[rooms[ESHK(shopkeeper)->shoproom].rtype - 8]);
  shopkeeper->bitflags |= M_IS_PEACEFUL; // NOTANGRY(shopkeeper) = 1;
  return true;
}


/* return 1 if paid successfully */
/*        0 if not enough money */
/*       -1 if object could not be found (but was paid) */
static Short dopayobj(bill_t *bp)
{
  obj_t *obj;
  Long ltmp;

  /* find the object on one of the lists */
  obj = bp_to_obj(bp);

  if (!obj) {
    message("BUG: Shopkeeper administration out of order.");
    setpaid();	/* be nice to the player */
    return 0;
  }

  if (!(obj->bitflags & O_IS_UNPAID) && !bp->useup) {
    message("BUG: Paid object on bill??");
    return 1;
  }
  obj->bitflags &= ~O_IS_UNPAID;
  ltmp = bp->price * bp->bquantity;
  if (ANGRY(shopkeeper)) ltmp += ltmp/3;
  if (you.ugold < ltmp) {
    StrPrintF(ScratchBuffer, "You don't have gold enough to pay %s.",
	      doname(obj));
    message(ScratchBuffer);
    obj->bitflags |= O_IS_UNPAID;
    return 0;
  }
  pay(ltmp, shopkeeper);
  StrPrintF(ScratchBuffer, "You bought %s for %ld gold piece%s",
	    doname(obj), ltmp, (ltmp == 1 ? "." : "s."));
  message(ScratchBuffer);
  if (bp->useup) {
    obj_t *otmp = billobjs;
    if (obj == billobjs)
      billobjs = obj->nobj;
    else {
      while (otmp && otmp->nobj != obj) otmp = otmp->nobj;
      if (otmp) otmp->nobj = obj->nobj;
      else message("BUG: Error in shopkeeper administration.");
    }
    free_me((VoidPtr) obj);
  }
  return 1;
}

/* routine called after dying (or quitting) with nonempty bill */
void paybill()
{
  if (shlevel == dlevel && shopkeeper && ESHK(shopkeeper)->billct) {
    addupbill();
    if (total > you.ugold){
      shopkeeper->mgold += you.ugold;
      you.ugold = 0;
      StrPrintF(ScratchBuffer, "%s comes and takes all your possessions.",
		Monnam(shopkeeper));
      message(ScratchBuffer);
    } else {
      you.ugold -= total;
      shopkeeper->mgold += total;
      StrPrintF(ScratchBuffer,
		"%s comes and takes the %ld zorkmids you owed him.",
		Monnam(shopkeeper), total);
      message(ScratchBuffer);
    }
    setpaid();	/* in case we create bones */
  }
}

// given an object ID, and a list of objects to search,
// return a ptr to the object that has that ID,
// or NULL if not found.
static obj_t * o_on(UInt id, obj_t *objchn) // was in invent.c
{
  while (objchn) {
    if (objchn->o_id == id) return objchn;
    objchn = objchn->nobj;
  }
  return NULL;
}

/* find obj on one of the lists */
obj_t * bp_to_obj(bill_t *bp)
{
  obj_t *obj;
  monst_t *mtmp;
  UShort id = bp->bo_id;

  if (bp->useup)
    obj = o_on(id, billobjs);
  else if (!(obj = o_on(id, invent)) &&
	   !(obj = o_on(id, fobj)) &&
	   !(obj = o_on(id, fcobj))) {
    for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon)
      if ((obj = o_on(id, mtmp->minvent)))
	break;
    for (mtmp = fallen_down ; mtmp ; mtmp = mtmp->nmon)
      if ((obj = o_on(id, mtmp->minvent)))
	break;
  }
  return obj;
}

/* called in hack.c (or whatever) when we pickup an object */
void addtobill(obj_t *obj)
{
  bill_t *bp;
  eshk_t *eshk = (shopkeeper ? ESHK(shopkeeper) : NULL);
  if (!inshop() ||
      (you.ux == eshk->shk.x && you.uy == eshk->shk.y) ||
      (you.ux == eshk->shd.x && you.uy == eshk->shd.y) ||
      onbill(obj)) /* perhaps we threw it away earlier */
    return;
  eshk = ESHK(shopkeeper);
  if (eshk->billct == BILLSZ) {
    // Well, I guess that's one way to address predefined limitations
    message("You got that for free!");
    return;
  }
  bp = &bill[eshk->billct];
  bp->bo_id = obj->o_id;
  bp->bquantity = obj->quantity;
  bp->useup = false;
  bp->price = getprice(obj);
  eshk->billct++;
  obj->bitflags |= O_IS_UNPAID;
}


void splitbill(obj_t *obj, obj_t *otmp)
{
  /* otmp has been split off from obj */
  bill_t *bp;
  Short tmp;
  bp = onbill(obj);
  if (!bp) {
    message("BUG: splitbill: not on bill?");
    return;
  }
  if (bp->bquantity < otmp->quantity) {
    message("BUG: Negative quantity on bill??");
    return;
  }
  if (bp->bquantity == otmp->quantity) {
    message("BUG: Zero quantity on bill??");
    return;
  }
  bp->bquantity -= otmp->quantity;

  /* addtobill(otmp); */ // this line was commented out when I found it.
  if (ESHK(shopkeeper)->billct == BILLSZ) otmp->bitflags &= ~O_IS_UNPAID;
  else {
    tmp = bp->price;
    bp = &bill[ESHK(shopkeeper)->billct];
    bp->bo_id = otmp->o_id;
    bp->bquantity = otmp->quantity;
    bp->useup = false;
    bp->price = tmp;
    ESHK(shopkeeper)->billct++;
  }
}

void subfrombill(struct obj *obj)
{
  Long ltmp;
  Short tmp;
  obj_t *otmp;
  bill_t *bp;
  eshk_t *eshk = (shopkeeper ? ESHK(shopkeeper) : NULL);
  if (!inshop() ||
      (you.ux == eshk->shk.x && you.uy == eshk->shk.y) ||
      (you.ux == eshk->shd.x && you.uy == eshk->shd.y))
    return;
  if ((bp = onbill(obj)) != 0) {
    obj->bitflags &= ~O_IS_UNPAID;
    if (bp->bquantity > obj->quantity) {
      otmp = (obj_t *) md_malloc(sizeof(obj_t));// newobj(0);
      *otmp = *obj;
      bp->bo_id = otmp->o_id = flags.ident++;
      otmp->quantity = (bp->bquantity -= obj->quantity);
      otmp->owt = 0;	/* superfluous */
      do_name(otmp, NULL); // otmp->onamelth = 0; // Undo name (if any)
      bp->useup = true;
      otmp->nobj = billobjs;
      billobjs = otmp;
      return;
    }
    eshk->billct--;
    *bp = bill[eshk->billct];
    return;
  }
  if (obj->bitflags & O_IS_UNPAID) {
    StrPrintF(ScratchBuffer, "%s didn't notice.", Monnam(shopkeeper));
    message(ScratchBuffer);
    obj->bitflags &= ~O_IS_UNPAID;
    return;		/* %% */
  }
  /* he dropped something of his own - probably wants to sell it */
  if ((shopkeeper->bitflags & (M_IS_ASLEEP | M_IS_FROZEN)) ||
      inroom(shopkeeper->mx,shopkeeper->my) != eshk->shoproom)
    return;
  if (eshk->billct == BILLSZ ||
      ((tmp = shtypes[rooms[eshk->shoproom].rtype-8]) && tmp != obj->olet) ||
      StrChr("_0", obj->olet)) {
    StrPrintF(ScratchBuffer, "%s seems not interested.", Monnam(shopkeeper));
    message(ScratchBuffer);
    return;
  }
  ltmp = getprice(obj) * obj->quantity;
  if (ANGRY(shopkeeper)) {
    ltmp /= 3;
    shopkeeper->bitflags |= M_IS_PEACEFUL; // NOTANGRY(shopkeeper) = 1;
  } else   ltmp /= 2;
  if (eshk->robbed) {
    if ((eshk->robbed -= ltmp) < 0)
      eshk->robbed = 0;
    message("Thank you for your contribution to restock this recently plundered shop.");
    return;
  }
  if (ltmp > shopkeeper->mgold)
    ltmp = shopkeeper->mgold;
  pay(-ltmp, shopkeeper);
  if (!ltmp)
    StrPrintF(ScratchBuffer,
	      "%s gladly accepts %s but cannot pay you at present.",
	      Monnam(shopkeeper), doname(obj));
  else
    StrPrintF(ScratchBuffer,
	      "You sold %s and got %ld gold piece%s",
	      doname(obj), ltmp, (ltmp == 1 ? "." : "s."));
  message(ScratchBuffer);
}


Short doinvbill(Short mode) /* 0: deliver count 1: paged */
{
  bill_t *bp;
  obj_t *obj;
  //  Long totused, thisused;
  //  Char buf[BUFSZ];

  if (mode == 0) {
    Short cnt = 0;

    if (shopkeeper)
      for (bp = bill ; bp - bill < ESHK(shopkeeper)->billct ; bp++)
	if (bp->useup ||
	    ((obj = bp_to_obj(bp)) && obj->quantity < bp->bquantity))
	  cnt++;
    return cnt;
  }

  if (!shopkeeper) {
    message("BUG: doinvbill: no shopkeeper?");
    return 0;
  }

  // XXXXX What one really ought to do is to pop up a form or something.
  /*
  set_pager(0);
  if (page_line("Unpaid articles already used up:") || page_line(""))
    goto quit;

  totused = 0;
  for (bp = bill; bp - bill < ESHK(shopkeeper)->billct; bp++) {
    obj = bp_to_obj(bp);
    if (!obj) {
      message("BUG: Bad shopkeeper administration.");
      return 0;
    }
    if (bp->useup || bp->bquantity > obj->quantity) {
      Short cnt, oquan, uquan;

      oquan = obj->quantity;
      uquan = (bp->useup ? bp->bquantity : bp->bquantity - oquan);
      thisused = bp->price * uquan;
      totused += thisused;
      obj->quantity = uquan;		// / * cheat doname * /
      (void) sprintf(buf, "x -  %s", doname(obj));
      obj->quantity = oquan;		// / * restore value * /
      for (cnt = 0; buf[cnt]; cnt++);
      while (cnt < 50)
	buf[cnt++] = ' ';
      (void) sprintf(&buf[cnt], " %5ld zorkmids", thisused);
      if (page_line(buf))
	goto quit;
    }
  }
  (void) sprintf(buf, "Total:%50ld zorkmids", totused);
  if (page_line("") || page_line(buf))
    goto quit;
  set_pager(1);
  return(0);
 quit:
  set_pager(2);
  */
  return(0);
}

static Short getprice(obj_t *obj)
{
  Short tmp, ac;

  switch(obj->olet) {
  case AMULET_SYM:
    tmp = 10*rnd(500);
    break;
  case TOOL_SYM:
    tmp = 10*rnd((obj->otype == EXPENSIVE_CAMERA) ? 150 : 30);
    break;
  case RING_SYM:
    tmp = 10*rnd(100);
    break;
  case WAND_SYM:
    tmp = 10*rnd(100);
    break;
  case SCROLL_SYM:
    tmp = 10*rnd(50);
#ifdef MAIL
    if (obj->otype == SCR_MAIL)
      tmp = rnd(5);
#endif MAIL
    break;
  case POTION_SYM:
    tmp = 10*rnd(50);
    break;
  case FOOD_SYM:
    tmp = 10*rnd(5 + (2000/realhunger()));
    break;
  case GEM_SYM:
    tmp = 10*rnd(20);
    break;
  case ARMOR_SYM:
    ac = ARM_BONUS(obj);
    if (ac <= -10)		/* probably impossible */
      ac = -9;
    tmp = 100 + ac*ac*rnd(10+ac);
    break;
  case WEAPON_SYM:
    if (obj->otype < BOOMERANG)
      tmp = 5*rnd(10);
    else if (obj->otype == LONG_SWORD ||
	     obj->otype == TWO_HANDED_SWORD)
      tmp = 10*rnd(150);
    else	tmp = 10*rnd(75);
    break;
  case CHAIN_SYM:
    message("Strange ..., carrying a chain?");
    // fall through
  case BALL_SYM:
    tmp = 10;
    break;
  default:
    tmp = 10000;
    break;
  }
  return tmp;
}


static Short realhunger()
{	/* not completely foolproof */
  Short tmp = you.uhunger;
  obj_t *otmp = invent;
  while (otmp) {
    if (otmp->olet == FOOD_SYM && !(otmp->bitflags & O_IS_UNPAID))
      tmp += objects[otmp->otype].nutrition;
    otmp = otmp->nobj;
  }
  return((tmp <= 0) ? 1 : tmp);
}

Boolean shkcatch(obj_t *obj)
{
  monst_t *shkp = shopkeeper;

  if (you.uinshop && shkp && !(shkp->bitflags & (M_IS_FROZEN | M_IS_ASLEEP)) &&
      you.dx && you.dy &&
      inroom(you.ux+you.dx, you.uy+you.dy) + 1 == you.uinshop &&
      shkp->mx == ESHK(shkp)->shk.x && shkp->my == ESHK(shkp)->shk.y &&
      you.ux == ESHK(shkp)->shd.x && you.uy == ESHK(shkp)->shd.y) {
    StrPrintF(ScratchBuffer, "%s nimbly catches the %s.",
	      Monnam(shkp), xname(obj));
    message(ScratchBuffer);
    obj->nobj = shkp->minvent;
    shkp->minvent = obj;
    return true;
  }
  return false;
}

/*
 * shk_move: return 1: he moved  0: he didnt  -1: let m_move do it
 * (what about "return 2" ???
 */
Short shk_move(monst_t *shkp)
{
  monst_t *mtmp;
  permonst_t *mdat = shkp->data;
  UChar gx,gy,omx,omy,nx,ny,nix,niy;
  Int8 appr,i;
  Short udist;
  Short z;
  Int8 shkroom,chi,chcnt,cnt;
  Boolean uondoor=false, satdoor, avoid=false, badinv;
  coord poss[9];
  Short info[9];
  obj_t *ib = NULL;

  omx = shkp->mx;
  omy = shkp->my;

  if ((udist = dist(omx,omy)) < 3) {
    if (ANGRY(shkp)) {
      hit_you(shkp, dice(mdat->damn, mdat->damd)+1);
      return 0;
    }
    if (ESHK(shkp)->following) {
      if (StrNCompare(ESHK(shkp)->customer, plname, PL_NSIZ)) {
	StrPrintF(ScratchBuffer, "Hello %s! I was looking for %s.",
		  plname, ESHK(shkp)->customer);
	message(ScratchBuffer);
	ESHK(shkp)->following = false;
	return 0;
      }
      if (!ESHK(shkp)->robbed) {	/* impossible? */
	ESHK(shkp)->following = false;
	return 0;
      }
      if (moves > followmsg+4) {
	StrPrintF(ScratchBuffer, "Hello %s! Didn't you forget to pay?",
		  plname);
	message(ScratchBuffer);
	followmsg = moves;
      }
      if (udist < 2)
	return 0;
    }
  }

  shkroom = inroom(omx,omy);
  appr = 1;
  gx = ESHK(shkp)->shk.x;
  gy = ESHK(shkp)->shk.y;
  satdoor = (gx == omx && gy == omy);
  if (ESHK(shkp)->following || ((z = holetime()) >= 0 && z*z <= udist)){
    gx = you.ux;
    gy = you.uy;
    if (shkroom < 0 || shkroom != inroom(you.ux,you.uy))
      if (udist > 4)
	return -1;	/* leave it to m_move */
  } else if (ANGRY(shkp)) {
    Long saveBlind = Blind;
    Blind = 0;
    if ((shkp->mcansee_and_blinded & M_CAN_SEE) && !Invis && cansee(omx,omy)) {
      gx = you.ux;
      gy = you.uy;
    }
    Blind = saveBlind;
    avoid = false;
  } else {
#define	GDIST(x,y)	((x-gx)*(x-gx)+(y-gy)*(y-gy))
    if (Invis)
      avoid = false;
    else {
      uondoor = (you.ux == ESHK(shkp)->shd.x &&
		 you.uy == ESHK(shkp)->shd.y);
      if (uondoor) {
	if (ESHK(shkp)->billct) {
	  StrPrintF(ScratchBuffer,
		    "Hello %s! Will you please pay before leaving?", plname);
	  message(ScratchBuffer);
	}
	badinv = (carrying(PICK_AXE) || carrying(ICE_BOX));
	if (satdoor && badinv)
	  return 0;
	avoid = !badinv;
      } else {
	avoid = (you.uinshop && dist(gx,gy) > 8);
	badinv = false;
      }

      if (((!ESHK(shkp)->robbed && !ESHK(shkp)->billct) || avoid)
	  && GDIST(omx,omy) < 3) {
	if (!badinv && !online(omx,omy))
	  return 0;
	if (satdoor)
	  appr = gx = gy = 0;
      }
    }
  }
  if (omx == gx && omy == gy)
    return 0;
  if (shkp->bitflags & M_IS_CONFUSED) {
    avoid = false;
    appr = 0;
  }
  nix = omx;
  niy = omy;
  cnt = mfindpos(shkp,poss,info,ALLOW_SSM);
  if (avoid && uondoor) {		/* perhaps we cannot avoid him */
    for (i=0; i<cnt; i++)
      if (!(info[i] & NOTONL)) goto notonl_ok;
    avoid = false;
  notonl_ok:
    ;
  }
  chi = -1;
  chcnt = 0;
  for (i = 0 ; i < cnt ; i++) {
    nx = poss[i].x;
    ny = poss[i].y;
    if (get_cell_type(floor_info[nx][ny]) == ROOM
	|| shkroom != ESHK(shkp)->shoproom
	|| ESHK(shkp)->following) {
#ifdef STUPID
      /* cater for stupid compilers */
      Short zz;
#endif STUPID
      if (uondoor && (ib = sobj_at(ICE_BOX, nx, ny))) {
	nix = nx; niy = ny; chi = i; break;
      }
      if (avoid && (info[i] & NOTONL))
	continue;
      if ((!appr && !rund(++chcnt)) ||
#ifdef STUPID
	  (appr && (zz = GDIST(nix,niy)) && zz > GDIST(nx,ny))
#else
	  (appr && GDIST(nx,ny) < GDIST(nix,niy))
#endif STUPID
	  ) {
	nix = nx;
	niy = ny;
	chi = i;
      }
    }
  }
  if (nix != omx || niy != omy) {
    if (info[chi] & ALLOW_M){
      mtmp = mon_at(nix,niy);
      if (hitmm(shkp,mtmp) == 1 && rund(3) &&
	  hitmm(mtmp,shkp) == 2) return 2;
      return 0;
    } else if (info[chi] & ALLOW_U){
      hit_you(shkp, dice(mdat->damn, mdat->damd)+1);
      return 0;
    }
    shkp->mx = nix;
    shkp->my = niy;
    pmon(shkp);
    if (ib) {
      unlink_obj(ib);//freeobj
      mpickobj(shkp, ib);
    }
    return 1;
  }
  return 0;
}


/* He is digging in the shop. */
void shopdig(Boolean fall)
{
  if (!fall) {
    if (you.utraptype == TT_PIT)
      message("\"Be careful, sir, or you might fall through the floor.\"");
    else
      message("\"Please, do not damage the floor here.\"");
  } else if (dist(shopkeeper->mx, shopkeeper->my) < 3) {
    obj_t *obj, *obj2;

    StrPrintF(ScratchBuffer, "%s grabs your backpack!", shkname(shopkeeper));
    message(ScratchBuffer);
    for (obj = invent ; obj ; obj = obj2) {
      obj2 = obj->nobj;
      if (obj->owornmask) continue;
      unlink_inv(obj); //freeinv(obj);
      obj->nobj = shopkeeper->minvent;
      shopkeeper->minvent = obj;
      if (obj->bitflags & O_IS_UNPAID)
	subfrombill(obj);
    }
  }
}


Boolean online(Short x, Short y)
{
  return(x==you.ux || y==you.uy ||
	 (x-you.ux)*(x-you.ux) == (y-you.uy)*(y-you.uy));
}

/* Does this monster follow me downstairs?  (shopkeeper or pet dog) */
Boolean follower(monst_t *mtmp)
{
  UShort mf = mtmp->bitflags;
  return( 
	 (mf & M_IS_TAME) || 
	 StrChr("1TVWZi&, ", mtmp->data->mlet) ||
	 ((mf & M_IS_SHOPKEEPER) && ESHK(mtmp)->following)
	 );
}


