/* Guild.c for ShadowWind MUD
 * Author: Desmond Daignault
 * Date:   19980512
 *
 */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"

extern struct descriptor_data *descriptor_list;

ACMD(do_guild)
{
}
ACMD(do_guildtitle)
{
}
ACMD(do_guildchat)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_GCHAT) && !IS_NPC(ch)) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME, CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT) && !IS_NPC(ch))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY)
        safe_snprintf(buf, MAX_STRING_LENGTH, "You quest-say, '%s'", argument);
      else
        strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      safe_snprintf(buf, MAX_STRING_LENGTH, "$n quest-says, '%s'", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i != ch->desc && PRF_FLAGGED(i->character, PRF_GCHAT))
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
ACMD(do_guildedit)
{
}
