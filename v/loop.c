/* v/loop.c
**
** This file is in the public domain.
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <setjmp.h>
#include <gmp.h>
#include <sigsegv.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <uv.h>
#include <errno.h>
#include <curses.h>
#include <termios.h>
#include <term.h>

#include "all.h"
#include "v/vere.h"

static jmp_buf Signal_buf;
#ifndef SIGSTKSZ
# define SIGSTKSZ 16384
#endif
static uint8_t Sigstk[SIGSTKSZ];

typedef enum {
  sig_none,
  sig_overflow,
  sig_interrupt,
  sig_terminate,
  sig_memory,
  sig_assert,
  sig_timer
} u2_kill;

volatile u2_kill Sigcause;            //  reasons for exception

static void
_lo_signal_handle_over(int emergency, stackoverflow_context_t scp)
{
  if ( u2_Critical ) {
    //  Careful not to grow the stack during critical sections.
    //
    write(2, "stack disaster\n", strlen("stack disaster" + 2));
    abort();
  }

#if 0
  if ( 1 == emergency ) {
    write(2, "stack emergency\n", strlen("stack emergency" + 2));
    abort();
  } else
#endif
  {
    Sigcause = sig_overflow;
    longjmp(Signal_buf, 1);
  }
}

static void
_lo_signal_handle_term(int x)
{
  if ( !u2_Critical ) {
    Sigcause = sig_terminate;
    u2_Host.liv = u2_no;
    longjmp(Signal_buf, 1);
  }
}

static void
_lo_signal_handle_intr(int x)
{
  if ( !u2_Critical ) {
    Sigcause = sig_interrupt;
    longjmp(Signal_buf, 1);
  }
}

static void
_lo_signal_handle_alrm(int x)
{
  if ( !u2_Critical ) {
    Sigcause = sig_timer;
    longjmp(Signal_buf, 1);
  }
}

/* _lo_signal_done():
*/
static void
_lo_signal_done()
{
  // signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGVTALRM, SIG_IGN);

  stackoverflow_deinstall_handler();
  {
    struct itimerval itm_u;

    timerclear(&itm_u.it_interval);
    timerclear(&itm_u.it_value);

    setitimer(ITIMER_VIRTUAL, &itm_u, 0);
  }
  u2_unix_ef_move();
}

/* _lo_signal_deep(): start deep processing; set timer for sec_w or 0.
*/
static void
_lo_signal_deep(c3_w sec_w)
{
  u2_unix_ef_hold();

  stackoverflow_install_handler(_lo_signal_handle_over, Sigstk, SIGSTKSZ);
  signal(SIGINT, _lo_signal_handle_intr);
  signal(SIGTERM, _lo_signal_handle_term);

  {
    struct itimerval itm_u;

    timerclear(&itm_u.it_interval);
    itm_u.it_value.tv_sec = sec_w;
    itm_u.it_value.tv_usec = 0;

    setitimer(ITIMER_VIRTUAL, &itm_u, 0);
  }
  signal(SIGVTALRM, _lo_signal_handle_alrm);
}

/* u2_loop_signal_memory(): end computation for out-of-memory.
*/
void
u2_loop_signal_memory()
{
  fprintf(stderr, "\r\nout of memory\r\n");
  Sigcause = sig_memory;
  longjmp(Signal_buf, 1);
}

/* _lo_init(): initialize I/O across the process.
*/
static void
_lo_init()
{
  u2_unix_io_init();
  u2_ames_io_init();
  u2_term_io_init();
  u2_http_io_init();
  u2_save_io_init();
  u2_batz_io_init();
}

/* _lo_talk(): bring up listeners across the process.
*/
static void
_lo_talk()
{
  u2_unix_io_talk();
  u2_ames_io_talk();
  u2_http_io_talk();
}

/* _lo_exit(): terminate I/O across the process.
*/
static void
_lo_exit(void)
{
  u2_unix_io_exit();
  u2_ames_io_exit();
  u2_term_io_exit();
  u2_http_io_exit();
  u2_save_io_exit();
  u2_batz_io_exit();
}

/* _lo_poll(): reset event flags across the process.
*/
static void
_lo_poll(void)
{
  u2_ames_io_poll();
  u2_http_io_poll();
  u2_term_io_poll();
  u2_save_io_poll();
  u2_unix_io_poll();
  u2_batz_io_poll();
}

#if 0
/* _lo_how(): print how.
*/
static const c3_c*
_lo_how(u2_noun how)
{
  switch ( how ) {
    default: c3_assert(0); break;

    case c3__ames: return "ames";
    case c3__batz: return "batz";
    case c3__term: return "cons";
    case c3__htcn: return "http-conn";
    case c3__htls: return "http-lisn";
    case c3__save: return "save";
    case c3__unix: return "unix";
  }
}
#endif

/* u2_lo_bail(): clean up all event state.
*/
void
u2_lo_bail(u2_reck* rec_u)
{
  fflush(stdout);
  _lo_exit();

  exit(1);
}
int c3_cooked() { _lo_exit(); return 0; }

/* _lo_tape(): dump a tape, old style.  Don't do this.
*/
static void
_lo_tape(u2_reck* rec_u, FILE* fil_u, u2_noun tep)
{
  u2_noun tap = tep;

  while ( u2_nul != tap ) {
    c3_c car_c;

    if ( u2h(tap) >= 127 ) {
      car_c = '?';
    } else car_c = u2h(tap);

    putc(car_c, fil_u);
    tap = u2t(tap);
  }
  u2z(tep);
}

/* _lo_wall(): dump a wall, old style.  Don't do this.
*/
static void
_lo_wall(u2_reck* rec_u, u2_noun wol)
{
  FILE* fil_u = u2_term_io_hija();
  u2_noun wal = wol;

  while ( u2_nul != wal ) {
    _lo_tape(rec_u, fil_u, u2k(u2h(wal)));

    putc(13, fil_u);
    putc(10, fil_u);

    wal = u2t(wal);
  }
  u2_term_io_loja(0);
  u2z(wol);
}

/* u2_lo_tank(): dump single tank.
*/
void
u2_lo_tank(c3_l tab_l, u2_noun tac)
{
  u2_lo_punt(tab_l, u2nc(tac, u2_nul));
}

/* u2_lo_punt(): dump tank list.
*/
void
u2_lo_punt(c3_l tab_l, u2_noun tac)
{
  u2_noun blu   = u2_term_get_blew(0);
  c3_l    col_l = u2h(blu);
  u2_noun cat   = tac;

  //  We are calling nock here, but hopefully need no protection.
  //
  while ( u2_yes == u2_cr_du(cat) ) {
    u2_noun wol = u2_dc("wash", u2nc(tab_l, col_l), u2k(u2h(cat)));

    _lo_wall(u2_Arv, wol);
    cat = u2t(cat);
  }
  u2z(tac);
  u2z(blu);
}

/* u2_lo_sway(): print trace.
*/
void
u2_lo_sway(c3_l tab_l, u2_noun tax)
{
  u2_noun mok = u2_dc("mook", 2, tax);

  u2_lo_punt(tab_l, u2k(u2t(mok)));
  u2z(mok);
}

/* _lo_soft(): standard soft wrapper.  unifies unix and nock errors.
**
**  Produces [%$ result] or [%error (list tank)].
*/
static u2_noun
_lo_soft(u2_reck* rec_u, c3_w sec_w, u2_funk fun_f, u2_noun arg)
{
  u2_noun hoe, pro, rop;

  u2_rl_leap(u2_Wire, c3__rock);

  //  system level setjmp, for signals
  //
  c3_assert(u2_nul == u2_wire_tax(u2_Wire));
  c3_assert(0 == u2_wire_kit_r(u2_Wire));

  //  stop signals
  //
  u2_unix_ef_hold();
  _lo_signal_deep(sec_w);

  if ( 0 != setjmp(Signal_buf) ) {
    u2_noun tax, pre, mok;

    //  return to blank state
    //
    _lo_signal_done();

    //  acquire trace and reset memory
    //
    tax = u2_wire_tax(u2_Wire);
    u2_rl_fall(u2_Wire);
    u2z(arg);

    tax = u2_rl_take(u2_Wire, tax);
    u2_wire_tax(u2_Wire) = u2_nul;
    mok = u2_dc("mook", 2, tax);

    //  other ugly disgusting cleanups
    {
      u2_wire_kit_r(u2_Wire) = 0;

      u2_hevx_be(u2_wire_hev_r(u2_Wire), u2_pryr, god) = 0;
      u2_hevx_at(u2_wire_hev_r(u2_Wire), lad) = 0;
    }

    switch ( Sigcause ) {
      default:            pre = c3__wyrd; break;
      case sig_none:      pre = c3__none; break;
      case sig_overflow:  pre = c3__over; break;
      case sig_interrupt: pre = c3__intr; break;
      case sig_terminate: pre = c3__term; break;
      case sig_memory:    pre = c3__full; break;
      case sig_assert:    pre = c3__lame; break;
      case sig_timer:     fprintf(stderr, "timer!!\r\n"); pre = c3__slow; break;
    }
    rop = u2nc(pre, u2k(u2t(mok)));
    u2z(mok);
    fprintf(stderr, "error computed\r\n");
    return rop;
  }

  if ( 0 != (hoe = u2_cm_trap()) ) {
    u2_noun mok;

    u2_rl_fall(u2_Wire);
    hoe = u2_rl_take(u2_Wire, hoe);
    u2_rl_flog(u2_Wire);

    mok = u2_dc("mook", 2, u2k(u2t(hoe)));
    rop = u2nc(u2k(u2h(hoe)), u2k(u2t(mok)));

    u2z(arg);
    u2z(hoe);
    u2z(mok);
  }
  else {
    u2_noun pro = fun_f(rec_u, arg);

    _lo_signal_done();
    u2_cm_done();

    u2_rl_fall(u2_Wire);
    pro = u2_rl_take(u2_Wire, pro);
    u2_rl_flog(u2_Wire);

    u2z(arg);
    rop = u2nc(u2_blip, pro);
  }
  pro = rop;

  return pro;
}

#if 0
/* _lo_hard(): standard hard wrapper.  Produces result and/or asserts.
*/
static u2_noun
_lo_hard(u2_reck* rec_u, u2_funk fun_f, u2_noun arg)
{
  u2_noun pro = _lo_soft(rec_u, 0, fun_f, arg);

  if ( u2_blip == u2h(pro) ) {
    u2_noun poo = u2k(u2t(pro));

    u2z(pro); return poo;
  }
  else {
    u2_lo_punt(2, u2k(u2t(pro)));
    u2z(pro);
    c3_assert(0);
  }
}
#endif

/* _lo_mung(): formula wrapper with gate and sample.
*/
  static u2_noun
  _lo_mung_in(u2_reck* rec_u, u2_noun gam)
  {
    u2_noun pro = u2_cn_mung(u2k(u2h(gam)), u2k(u2t(gam)));

    u2z(gam); return pro;
  }
static u2_noun
_lo_mung(u2_reck* rec_u, c3_w sec_w, u2_noun gat, u2_noun sam)
{
  u2_noun gam = u2nc(gat, sam);

  return _lo_soft(rec_u, 0, _lo_mung_in, gam);
}

/* _lo_save(): log an ovum at the present time.
*/
static void
_lo_save(u2_reck* rec_u, u2_noun ovo)
{
  rec_u->roe = u2nc(ovo, rec_u->roe);
}

/* _lo_pike(): poke with floating core.
*/
static u2_noun
_lo_pike(u2_reck* rec_u, u2_noun ovo, u2_noun cor)
{
  u2_noun fun = u2_cn_nock(cor, u2k(u2_cx_at(42, cor)));
  u2_noun sam = u2nc(u2k(rec_u->now), ovo);

  return _lo_mung(rec_u, 0, fun, sam);
}

/* _lo_sure(): apply and save an input ovum and its result.
*/
static void
_lo_sure(u2_reck* rec_u, u2_noun ovo, u2_noun vir, u2_noun cor)
{
  //  Whatever worked, save it.  (XX - should be concurrent with execute.)
  //  We'd like more events that don't change the state but need work here.
  {
    u2_mug(cor);
    u2_mug(rec_u->roc);

    if ( u2_no == u2_sing(cor, rec_u->roc) ) {
      _lo_save(rec_u, u2k(ovo));

      u2z(rec_u->roc);
      rec_u->roc = cor;
    }
    else {
      u2z(cor);
    }
    u2z(ovo);
  }
}

/* _lo_lame(): handle an application failure.
*/
static void
_lo_lame(u2_reck* rec_u, u2_noun ovo, u2_noun why, u2_noun tan)
{
  u2_noun bov, gon;

#if 1
  {
    c3_c* oik_c = u2_cr_string(u2h(u2t(ovo)));

    // uL(fprintf(uH, "lame: %s\n", oik_c));
    free(oik_c);
  }
#endif

  //  Formal error in a network packet generates a hole card.
  //
  //  There should be a separate path for crypto failures,
  //  to prevent timing attacks, but isn't right now.  To deal
  //  with a crypto failure, just drop the packet.
  //
  if ( (c3__exit == why) && (c3__hear == u2h(u2t(ovo))) ) {
    u2_lo_punt(2, u2_ckb_flop(u2k(tan)));

    bov = u2nc(u2k(u2h(ovo)), u2nc(c3__hole, u2k(u2t(u2t(ovo)))));
    u2z(why);
  }
  else {
    bov = u2nc(u2k(u2h(ovo)), u2nt(c3__crud, why, u2k(tan)));
    u2_hevn_at(lad) = u2_nul;
  }
  // u2_lo_show("data", u2k(u2t(u2t(ovo))));

  u2z(ovo);

  gon = _lo_soft(rec_u, 0, u2_reck_poke, u2k(bov));
  if ( u2_blip == u2h(gon) ) {
    _lo_sure(rec_u, bov, u2k(u2h(u2t(gon))), u2k(u2t(u2t(gon))));

    u2z(gon);
  }
  else {
    u2z(gon);
    {
      u2_noun vab = u2nc(u2k(u2h(bov)),
                         u2nc(c3__warn, u2_ci_tape("crude crash!")));
      u2_noun nog = _lo_soft(rec_u, 0, u2_reck_poke, u2k(vab));

      if ( u2_blip == u2h(nog) ) {
        _lo_sure(rec_u, vab, u2k(u2h(u2t(nog))), u2k(u2t(u2t(nog))));
        u2z(nog);
      }
      else {
        u2z(nog);
        u2z(vab);

        uL(fprintf(uH, "crude: all delivery failed!\n"));
      }
    }
  }
}

static void _lo_punk(u2_reck* rec_u, u2_noun ovo);

/* _lo_nick(): transform enveloped packets, [vir cor].
*/
static u2_noun
_lo_nick(u2_reck* rec_u, u2_noun vir, u2_noun cor)
{
  if ( u2_nul == vir ) {
    return u2nt(u2_blip, vir, cor);
  }
  else {
    u2_noun i_vir = u2h(vir);
    u2_noun pi_vir, qi_vir;
    u2_noun vix;

    if ( (u2_yes == u2_cr_cell((i_vir=u2h(vir)), &pi_vir, &qi_vir)) &&
         (u2_yes == u2du(qi_vir)) &&
         (c3__hear == u2h(qi_vir)) )
    {
      u2_noun gon;

      gon = _lo_pike(rec_u, u2k(i_vir), cor);
      if ( u2_blip != u2h(gon) ) {
        u2z(vir);
        return gon;
      }
      else {
        u2_noun viz;

        vix = u2k(u2h(u2t(gon)));
        cor = u2k(u2t(u2t(gon)));
        u2z(gon);

        viz = u2_ckb_weld(vix, u2k(u2t(vir)));
        u2z(vir);

        return _lo_nick(rec_u, viz, cor);
      }
    }
    else {
      u2_noun nez = _lo_nick(rec_u, u2k(u2t(vir)), cor);

      if ( u2_blip != u2h(nez) ) {
        u2z(vir);
        return nez;
      } else {
        u2_noun viz;

        viz = u2nc(u2k(i_vir), u2k(u2h(u2t(nez))));
        cor = u2k(u2t(u2t(nez)));

        u2z(vir);
        u2z(nez);

        return u2nt(u2_blip, viz, cor);
      }
    }
  }
}

/* _lo_punk(): insert and apply an input ovum (unprotected).
*/
static void
_lo_punk(u2_reck* rec_u, u2_noun ovo)
{
  // c3_c* txt_c = u2_cr_string(u2h(u2t(ovo)));
  c3_w sec_w;
  // static c3_w num_w;
  u2_noun gon;

  // uL(fprintf(uH, "punk: %s: %d\n", u2_cr_string(u2h(u2t(ovo))), num_w++));

  //  XX this is wrong - the timer should be on the original hose.
  //
  if ( (c3__term == u2h(u2t(u2h(ovo)))) ||
       (c3__batz == u2h(u2t(u2h(ovo)))) ) {
    sec_w = 0;
  } else sec_w = 60;

  //  Control alarm loops.
  //
  if ( c3__wake != u2h(u2t(ovo)) ) {
    u2_Host.beh_u.run_w = 0;
  }

  gon = _lo_soft(rec_u, sec_w, u2_reck_poke, u2k(ovo));

  if ( u2_blip != u2h(gon) ) {
    u2_noun why = u2k(u2h(gon));
    u2_noun tan = u2k(u2t(gon));

    u2z(gon);
    _lo_lame(rec_u, ovo, why, tan);
  }
  else {
    u2_noun vir = u2k(u2h(u2t(gon)));
    u2_noun cor = u2k(u2t(u2t(gon)));
    u2_noun nug;

    u2z(gon);
    nug = _lo_nick(rec_u, vir, cor);

    if ( u2_blip != u2h(nug) ) {
      u2_noun why = u2k(u2h(nug));
      u2_noun tan = u2k(u2t(nug));

      u2z(nug);
      _lo_lame(rec_u, ovo, why, tan);
    }
    else {
      vir = u2k(u2h(u2t(nug)));
      cor = u2k(u2t(u2t(nug)));

      u2z(nug);
      _lo_sure(rec_u, ovo, vir, cor);
    }
  }
  // uL(fprintf(uH, "punk oot %s\n", txt_c));
}

/* _lo_suck(): past failure.
*/
static void
_lo_suck(u2_reck* rec_u, u2_noun ovo, u2_noun gon)
{
  uL(fprintf(uH, "sing: ovum failed!\n"));
  {
    c3_c* hed_c = u2_cr_string(u2h(u2t(ovo)));

    uL(fprintf(uH, "fail %s\n", hed_c));
    free(hed_c);
  }

  u2_lo_punt(2, u2_ckb_flop(u2k(u2t(gon))));
  u2_loom_exit();
  _lo_exit();

  exit(1);
}

/* _lo_sing(): replay ovum from the past, time already set.
*/
static void
_lo_sing(u2_reck* rec_u, u2_noun ovo)
{
  u2_noun gon = _lo_soft(rec_u, 0, u2_reck_poke, u2k(ovo));

  if ( u2_blip != u2h(gon) ) {
    _lo_suck(rec_u, ovo, gon);
  }
  else {
    u2_noun vir = u2k(u2h(u2t(gon)));
    u2_noun cor = u2k(u2t(u2t(gon)));
    u2_noun nug;

    u2z(gon);
    nug = _lo_nick(rec_u, vir, cor);

    if ( u2_blip != u2h(nug) ) {
      _lo_suck(rec_u, ovo, nug);
    }
    else {
      vir = u2h(u2t(nug));
      cor = u2k(u2t(u2t(nug)));

      while ( u2_nul != vir ) {
        u2_noun fex = u2h(vir);
        u2_noun fav = u2t(fex);

        if ( (c3__init == u2h(fav)) || (c3__inuk == u2h(fav)) ) {
          rec_u->own = u2nc(u2k(u2t(fav)), rec_u->own);
        }
        vir = u2t(vir);
      }
      u2z(nug);
      u2z(rec_u->roc);
      rec_u->roc = cor;
    }
    u2z(ovo);
  }
}

/* _lo_work(): work in rec_u.
*/
static void
_lo_work(u2_reck* rec_u)
{
  u2_cart* egg_u;
  u2_noun  ovo;
  u2_noun  ova;
  u2_noun  ovi;

  //  Apply effects from just-committed events, and delete finished events.
  //
  while ( rec_u->ova.egg_u ) {
    egg_u = rec_u->ova.egg_u;

    if ( u2_yes == egg_u->did ) {
      ovo = egg_u->egg;

      if ( egg_u == rec_u->ova.geg_u ) {
        c3_assert(egg_u->nex_u == 0);
        rec_u->ova.geg_u = rec_u->ova.egg_u = 0;
        free(egg_u);
      }
      else {
        c3_assert(egg_u->nex_u != 0);
        rec_u->ova.egg_u = egg_u->nex_u;
        free(egg_u);
      }

      if ( u2_yes == egg_u->cit ) {
        rec_u->ent_w++;
        u2_reck_kick(rec_u, u2k(ovo));
      }
      else {
        //  We poked an event, but Raft failed to persist it.
        //  TODO: gracefully recover.
        uL(fprintf(uH, "vere: event executed but not persisted\n"));
        c3_assert(0);
      }
      u2z(ovo);
    }
    else break;
  }

  //  Poke pending events, leaving the poked events and errors on rec_u->roe.
  //
  {
    ova = rec_u->roe;
    rec_u->roe = u2_nul;

    ovi = ova;
    while ( u2_nul != ovi ) {
      _lo_punk(rec_u, u2k(u2h(ovi)));  // XX unnecessary u2k?
      ovi = u2t(ovi);
    }

    u2z(ova);
  }

  //  Cartify, jam, and encrypt this batch of events. Take a number, Raft will
  //  be with you shortly.
  {
    c3_w    len_w;
    c3_w*   bob_w;
    u2_noun ron;

    ova = u2_ckb_flop(rec_u->roe);
    rec_u->roe = u2_nul;

    ovi = ova;
    while ( u2_nul != ovi ) {
      egg_u = malloc(sizeof(*egg_u));
      egg_u->nex_u = 0;
      egg_u->cit = u2_no;
      egg_u->did = u2_no;
      egg_u->egg = u2k(u2h(ovi));
      ovi = u2t(ovi);

      ron = u2_cke_jam(u2k(egg_u->egg));
      c3_assert(rec_u->key);
      ron = u2_dc("en:crya", u2k(rec_u->key), ron);

      len_w = u2_cr_met(5, ron);
      bob_w = malloc(len_w * 4L);

      egg_u->ent_w = u2_raft_push(u2R, bob_w, len_w);

      if ( 0 == rec_u->ova.geg_u ) {
        c3_assert(0 == rec_u->ova.egg_u);
        rec_u->ova.geg_u = rec_u->ova.egg_u = egg_u;
      }
      else {
        c3_assert(0 == rec_u->ova.geg_u->nex_u);
        rec_u->ova.geg_u->nex_u = egg_u;
        rec_u->ova.geg_u = egg_u;
      }
    }
    u2z(ova);
  }
}

/* u2_lo_open(): begin callback processing.
*/
void
u2_lo_open(void)
{
  //  update time
  //
  u2_reck_time(u2A);
}

/* u2_lo_shut(): end callback processing.
*/
void
u2_lo_shut(u2_bean inn)
{
  // u2_lo_grab("lo_shut a", u2_none);

  //  process actions
  //
  _lo_work(u2A);

  // u2_lo_grab("lo_shut b", u2_none);

  //  update time
  //
  u2_reck_time(u2A);

  // u2_lo_grab("lo_shut c", u2_none);

  //  for input operations, poll fs (XX not permanent)
  //
  if ( u2_yes == inn ) {
    u2_unix_ef_look();
  }

  // u2_lo_grab("lo_shut d", u2_none);

  //  clean shutdown
  //
  if ( u2_no == u2_Host.liv ) {
    //  direct save and die
    //
    u2_cm_purge();
    // u2_lo_grab("lo_exit", u2_none);
    u2_loom_save(u2A->ent_w);
    u2_loom_exit();
    _lo_exit();

    exit(0);
  }
  else {
    //  poll arvo to generate any event binding changes
    //
    _lo_poll();
  }
}

#if 0
//  _lo_bench_noop(): benchmark no-op events.
//
static void
_lo_bench_noop(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_reck_plan(u2A, u2nq(c3__gold, c3__term, 1, u2_nul),
                      u2nc(c3__noop, u2_nul));
  }

  _lo_work(u2A);
}

//  _lo_bench_scot_p(): benchmark prettyprint.
//
static void
_lo_bench_scot_p(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_noun soc = u2_dc("scot", 'p', u2k(u2A->now));

    u2z(soc);
  }
}

//  _lo_bench_slay_p(): benchmark prettyprint.
//
static void
_lo_bench_slay_p(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_noun soc = u2_dc("scot", 'p', u2k(u2A->now));
    u2_noun dub = u2_do("slay", soc);

    u2z(dub);
  }
}

//  _lo_bench_scot_da(): benchmark prettyprint.
//
static void
_lo_bench_scot_da(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_noun soc = u2_dc("scot", c3__da, u2k(u2A->now));

    u2z(soc);
  }
}

//  _lo_bench_dec(): benchmark decrement.
//
static void
_lo_bench_dec(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_noun soc = u2_do("dec", u2k(u2A->now));

    u2z(soc);
  }
}

//  _lo_bench_scot_ud(): benchmark prettyprint.
//
static void
_lo_bench_scot_ud(c3_w num_w)
{
  c3_w i_w;

  for ( i_w = 0; i_w < num_w; i_w++ ) {
    u2_noun soc = u2_dc("scot", c3__ud, u2k(u2A->now));

    u2z(soc);
  }
}

//  _lo_bench(): lo-tech profiling.
//
static void
_lo_bench(const c3_c* lab_c, void (*fun)(c3_w), c3_w num_w)
{
  u2_noun old, new;

  uL(fprintf(uH, "bench: %s: start...\n", lab_c));
  u2_reck_time(u2A);
  old = u2k(u2A->now);

  fun(num_w);

  u2_reck_time(u2A);
  new = u2k(u2A->now);
  {
    c3_w tms_w = (c3_w)u2_time_gap_ms(old, new);

    if ( tms_w > (10 * num_w) ) {
      uL(fprintf(uH, "bench: %s*%d: %d ms, %d ms each.\n",
                      lab_c, num_w, tms_w, (tms_w / num_w)));
    }
    else {
      uL(fprintf(uH, "bench: %s*%d: %d ms, %d us each.\n",
                      lab_c, num_w, tms_w, ((tms_w * 1000) / num_w)));
    }
  }
}
#endif

/*  u2_lo_show(): generic noun print.
*/
void
u2_lo_show(c3_c* cap_c, u2_noun nun)
{
  u2_noun pav   = u2_dc("pave", c3__noun, nun);
  c3_c*   txt_c = (c3_c*)u2_cr_tape(pav);

  fprintf(stderr, "%s: %s\r\n", cap_c, txt_c);
  u2z(pav);
  free(txt_c);
}

static void
_lo_slow()
{
#if 0
  _lo_bench("scot %p", _lo_bench_scot_p, 256);
  _lo_bench("scot %da", _lo_bench_scot_da, 256);
  _lo_bench("scot %ud", _lo_bench_scot_ud, 256);
  _lo_bench("slay %p", _lo_bench_slay_p, 256);
  _lo_bench("noop", _lo_bench_noop, 256);
#endif
}

/* u2_lo_boot(): restore or create pier.
*/
void
u2_lo_boot()
{
  uv_loop_t* lup_u = uv_default_loop();

  u2_Host.lup_u = lup_u;

  signal(SIGPIPE, SIG_IGN);     //  pipe, schmipe
  // signal(SIGIO, SIG_IGN);    //  linux is wont to produce for some reason

  _lo_init();
  u2_raft_boot();
}

/* u2_lo_loop(): begin main event loop.
*/
void
u2_lo_loop(u2_reck* rec_u)
{
  _lo_talk();
  {
    u2_unix_ef_look();
    u2_reck_plan(rec_u, u2nt(c3__gold, c3__ames, u2_nul),
                        u2nc(c3__kick, u2k(rec_u->now)));
  }
  _lo_poll();

#if 1
  u2_loom_save(rec_u->ent_w);

  u2_Host.sav_u.ent_w = rec_u->ent_w;
#endif

  if ( u2_yes == u2_Host.ops_u.nuu ) {
    u2_term_ef_boil(1);
  }

#if 1
  _lo_slow();
#endif

  uv_run(u2L, UV_RUN_DEFAULT);
}

/* _lo_mark_reck(): mark a reck.
*/
static c3_w
_lo_mark_reck(u2_reck* rec_u)
{
  c3_w siz_w = 0;
  c3_w egg_w;

  siz_w += u2_cm_mark_noun(rec_u->ken);
  siz_w += u2_cm_mark_noun(rec_u->roc);

  siz_w += u2_cm_mark_noun(rec_u->yot);
  siz_w += u2_cm_mark_noun(rec_u->now);
  siz_w += u2_cm_mark_noun(rec_u->wen);
  siz_w += u2_cm_mark_noun(rec_u->sen);
  siz_w += u2_cm_mark_noun(rec_u->own);
  siz_w += u2_cm_mark_noun(rec_u->roe);
  siz_w += u2_cm_mark_noun(rec_u->key);

  {
    u2_cart* egg_u;

    egg_w = 0;
    for ( egg_u = rec_u->ova.egg_u; egg_u; egg_u = egg_u->nex_u ) {
      egg_w += u2_cm_mark_noun(egg_u->egg);
    }
    siz_w += egg_w;
  }
#if 0
  fprintf(stderr, "ken %d, roc %d, yot %d, roe %d, egg %d\r\n",
                   ken_w, roc_w, yot_w, roe_w, egg_w);
#endif
  return siz_w;
}

/* _lo_mark(): mark the whole vere system.
*/
static c3_w
_lo_mark()
{
  c3_w siz_w;

  siz_w = u2_cm_mark_internal();
  siz_w += _lo_mark_reck(u2_Host.arv_u);

  return siz_w;
}

/* _lo_word(): print a word to stderr.
*/
static void
_lo_word(c3_w wod_w)
{
  u2_bean top = u2_yes;

  if ( wod_w / (1000 * 1000 * 1000) ) {
    uL(fprintf(uH, "%u.", wod_w / (1000 * 1000 * 1000)));
    wod_w %= (1000 * 1000 * 1000);
    top = u2_no;
  }
  if ( wod_w / (1000 * 1000) ) {
    uL(fprintf(uH, ((top == u2_yes) ? "%u." : "%03u."),
                     wod_w / (1000 * 1000)));
    wod_w %= (1000 * 1000);
    top = u2_no;
  }
  if ( wod_w / 1000 ) {
    uL(fprintf(uH, ((top == u2_yes) ? "%u." : "%03u."), wod_w / 1000));
    wod_w %= 1000;
    top = u2_no;
  }
  uL(fprintf(uH, ((top == u2_yes) ? "%u" : "%03u"), wod_w));
}

/* u2_lo_grab(): garbage-collect the world, plus roots.
*/
void
u2_lo_grab(c3_c* cap_c, u2_noun som, ...)
{
  c3_w siz_w, lec_w;

  siz_w = _lo_mark();
  {
    va_list vap;
    u2_noun tur;

    va_start(vap, som);

    if ( som != u2_none ) {
      siz_w += u2_cm_mark_noun(som);

      while ( u2_none != (tur = va_arg(vap, u2_noun)) ) {
        siz_w += u2_cm_mark_noun(tur);
      }
    }
    va_end(vap);
  }
  lec_w = u2_cm_sweep(siz_w);

  // if ( lec_w || (u2_yes == u2_Flag_Verbose) )
  if ( lec_w  || !strcmp("init", cap_c) ) {
    uL(fprintf(uH, "%s: gc: ", cap_c));
    if ( lec_w ) {
      _lo_word(4 * lec_w);
      uL(fprintf(uH, " bytes shed; "));
    }
    _lo_word(4 * siz_w);
    uL(fprintf(uH, " bytes live\n"));

#if 0
    if ( lec_w ) {
      uL(fprintf(uH, "zero garbage tolerance!\n"));
      _lo_exit();
      c3_assert(0);
      exit(1);
    }
#endif
  }
  u2_wire_lan(u2_Wire) = u2_yes;
}
