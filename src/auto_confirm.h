#include <stdint.h>
#include <string.h>
/* OK-only dialogs (mode 3) that only report success, answered automatically:
   - the data-access icon notice (opened by 0x5a81a0, +0x284 == 1);
   - hd_title (kind 0x12) id 10 "Load successful.";
   - hd_saveload (kind 0x11) id 14 "Save successful.", 19 "Saved data has been deleted.",
     21 "New saved data has been created.".
   Kind/id are +0x290/+0x294, written by 0x5a8680, which sets +0x284 to 0. Errors stay manual. */
static int auto_confirm_dialog(const unsigned char *dialog) {
 uint32_t kind, id;
 if(dialog[0x284]==1)return 1;
 if(dialog[0x284]!=0)return 0;
 memcpy(&kind,dialog+0x290,4);memcpy(&id,dialog+0x294,4);
 if(kind==0x12)return id==0x0a;
 if(kind==0x11)return id==0x0e || id==0x13 || id==0x15;
 return 0;
}
/* The decide press counts only after the minimum display time (+0x278, 0.5 s; elapsed +0x274).
   Once `seconds` have passed, satisfy it and report that the press can be sent. */
static int auto_confirm_ready(unsigned char *dialog, float seconds) {
 float elapsed, minimum;
 memcpy(&elapsed,dialog+0x274,4);memcpy(&minimum,dialog+0x278,4);
 if(elapsed<seconds)return 0;
 if(elapsed<minimum)memcpy(dialog+0x274,&minimum,4);
 return 1;
}
