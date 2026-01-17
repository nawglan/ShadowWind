/* ************************************************************************
 *   File: screen.h                                      Part of CircleMUD *
 *  Usage: header file with ANSI color codes for online color              *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define KNRM  "\x1B[0m"
#define KGRE  "\x1B[0;30m"
#define KRED  "\x1B[0;31m"
#define KGRN  "\x1B[0;32m"
#define KYEL  "\x1B[0;33m"
#define KBLU  "\x1B[0;34m"
#define KMAG  "\x1B[0;35m"
#define KCYN  "\x1B[0;36m"
#define KWHT  "\x1B[0;37m"
#define KBRED "\x1B[31;1m"
#define KBGRN "\x1B[32;1m"
#define KBYEL "\x1B[33;1m"
#define KBBLU "\x1B[34;1m"
#define KBMAG "\x1B[35;1m"
#define KBCYN "\x1B[36;1m"
#define KBWHT "\x1B[37;1m"
#define KNUL  ""

/* conditional color.  pass it a pointer to a char_data and a color level. */
#define C_OFF          0
#define C_SPR          1
#define C_NRM          2
#define C_CMP          3
#define _clrlevel(ch)  ((PRF_FLAGGED((ch), PRF_COLOR_1) ? 1 : 0) + (PRF_FLAGGED((ch), PRF_COLOR_2) ? 2 : 0))
#define clr(ch, lvl)   (_clrlevel(ch) >= (lvl))
#define CCNRM(ch, lvl) (clr((ch), (lvl)) ? KNRM : KNUL)
#define CCGRE(ch, lvl) (clr((ch), (lvl)) ? KGRE : KNUL)
#define CCRED(ch, lvl) (clr((ch), (lvl)) ? KRED : KNUL)
#define CCGRN(ch, lvl) (clr((ch), (lvl)) ? KGRN : KNUL)
#define CCYEL(ch, lvl) (clr((ch), (lvl)) ? KYEL : KNUL)
#define CCBLU(ch, lvl) (clr((ch), (lvl)) ? KBLU : KNUL)
#define CCMAG(ch, lvl) (clr((ch), (lvl)) ? KMAG : KNUL)
#define CCCYN(ch, lvl) (clr((ch), (lvl)) ? KCYN : KNUL)
#define CCWHT(ch, lvl) (clr((ch), (lvl)) ? KWHT : KNUL)
#define CBRED(ch, lvl) (clr((ch), (lvl)) ? KBRED : KNUL)
#define CBGRN(ch, lvl) (clr((ch), (lvl)) ? KBGRN : KNUL)
#define CBYEL(ch, lvl) (clr((ch), (lvl)) ? KBYEL : KNUL)
#define CBBLU(ch, lvl) (clr((ch), (lvl)) ? KBBLU : KNUL)
#define CBMAG(ch, lvl) (clr((ch), (lvl)) ? KBMAG : KNUL)
#define CBCYN(ch, lvl) (clr((ch), (lvl)) ? KBCYN : KNUL)
#define CBWHT(ch, lvl) (clr((ch), (lvl)) ? KBWHT : KNUL)
#define CBGRE(ch, lvl) (clr((ch), (lvl)) ? C_D_GREY : KNUL)

#define COLOR_LEV(ch) (_clrlevel(ch))

#define QNRM  CCNRM(ch, C_SPR)
#define QGRE  CCGRE(ch, C_SPR)
#define QRED  CCRED(ch, C_SPR)
#define QGRN  CCGRN(ch, C_SPR)
#define QYEL  CCYEL(ch, C_SPR)
#define QBLU  CCBLU(ch, C_SPR)
#define QMAG  CCMAG(ch, C_SPR)
#define QCYN  CCCYN(ch, C_SPR)
#define QWHT  CCWHT(ch, C_SPR)
#define QBRED CBRED(ch, C_SPR)
#define QBGRN CBGRN(ch, C_SPR)
#define QBYEL CBYEL(ch, C_SPR)
#define QBBLU CBBLU(ch, C_SPR)
#define QBMAG CBMAG(ch, C_SPR)
#define QBCYN CBCYN(ch, C_SPR)
#define QBWHT CBWHT(ch, C_SPR)

/* Meith's color codes.	*/

#define CLEAR       "[0m"        /* Resets Colour	*/
#define C_RED       "[0m[0;31m" /* Normal Colours	*/
#define C_GREEN     "[0m[0;32m"
#define C_YELLOW    "[0m[0;33m"
#define C_BLUE      "[0m[0;34m"
#define C_MAGENTA   "[0m[0;35m"
#define C_CYAN      "[0m[0;36m"
#define C_WHITE     "[0m[0;37m"
#define C_D_GREY    "[0m[1;30m" /* Light Colors		*/
#define C_B_RED     "[0m[1;31m"
#define C_B_GREEN   "[0m[1;32m"
#define C_B_YELLOW  "[0m[1;33m"
#define C_B_BLUE    "[0m[1;34m"
#define C_B_MAGENTA "[0m[1;35m"
#define C_B_CYAN    "[0m[1;36m"
#define C_B_WHITE   "[0m[1;37m"
#define FLASH       "[5m"
#define UNDERLINE   "[4m"
