#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "save_sound.h"
#include "typewriter.h"
#include "patches.h"
#include "stubs.h"
_Static_assert(PATCH_COUNT*STUB_SIZE<=8192,"Stub page too small");
static float wait_seconds=0.3f, fade_speed=2.0f, recap_speed=4.0f, recap_interval=2.0f, partner_speed=2.0f, save_text_speed=4.0f, save_text_hold=0.3f;
static DWORD save_ui;
static struct typing_sound sound_state;
static int skip_typewriter_intro=1;
static float dumbwaiter_speed=5.0f;
typedef int (__cdecl *script_command)(BYTE *);
/* Dumbwaiter: an UpCut close-up whose script reaches the item screen, from UpCutChange
   until UpCutStart returns to the room camera. Keyed by context, script id and buffer. */
static BYTE *waiter_ctx; static DWORD waiter_script, waiter_buffer; static int in_upcut, waiter_active;
static int waiter_matches(BYTE *ctx) {
 return ctx==waiter_ctx && *(DWORD *)(ctx+4)==waiter_script && *(DWORD *)(ctx+8)==waiter_buffer;
}
static void waiter_track(BYTE *ctx) {
 if(!waiter_matches(ctx)){waiter_ctx=ctx;waiter_script=*(DWORD *)(ctx+4);waiter_buffer=*(DWORD *)(ctx+8);in_upcut=waiter_active=0;}
}
/* Scan from this UpCutChange to the closing UpCutStart for the dumbwaiter item screen
   (Sub_ReadyGet / Sub_ReadyGetParam). Rows: 2-byte big-endian opcode (table index + 1) + arguments. */
static int dumbwaiter_ahead(BYTE *ctx) {
 const BYTE *code=*(const BYTE **)(ctx+8);DWORD at=*(DWORD *)(ctx+0xc);
 for(int n=0;n<64;n++){
  DWORD op=(DWORD)code[at]<<8|code[at+1];
  if(op==0 || op>313 || op==86+1)return 0;
  if(op==116+1 || op==117+1)return 1;
  at+=2+((DWORD (__cdecl *)(const char *))0x57cc10)(*(const char **)(0xcd57e0+(op-1)*12+8));
 }
 return 0;
}
static int __cdecl upcut_change(BYTE *ctx){waiter_track(ctx);in_upcut=1;waiter_active=dumbwaiter_ahead(ctx);return ((script_command)0x57c5b0)(ctx);}
static int __cdecl upcut_start(BYTE *ctx){if(waiter_matches(ctx))in_upcut=waiter_active=0;return ((script_command)0x57b1d0)(ctx);}
static int __cdecl sub_ready_temp(BYTE *ctx){int done=((script_command)0x568e30)(ctx);if(waiter_matches(ctx) && in_upcut)waiter_active=1;return done;}
static int __cdecl sub_get_result(BYTE *ctx){int done=((script_command)0x568eb0)(ctx);if(waiter_matches(ctx) && in_upcut)waiter_active=1;return done;}
/* frame counts ctx+0x10 down by the tick delta; remove (speed-1) extra deltas while waiting. */
static int __cdecl script_frame(BYTE *ctx) {
 int done=((script_command)0x576350)(ctx);
 if(!done && waiter_active && waiter_matches(ctx))*(float *)(ctx+0x10)-=(*(float **)0xe2d904)[0x68/4]*(dumbwaiter_speed-1);
 return done;
}
/* se waits for its sound to end; inside the window, continue while the sound keeps playing. */
static int __cdecl script_se(BYTE *ctx) {
 int done=((script_command)0x57a8e0)(ctx);
 if(done || !waiter_active || !waiter_matches(ctx))return done;
 *(DWORD *)(ctx+0x38)=0;
 ((void (__cdecl *)(BYTE *))0x57cef0)(ctx);
 return 1;
}
/* Inside the window, leave the idle message UI closed; the command then completes next tick. */
static void __attribute__((fastcall)) dumbwaiter_message(DWORD *manager, void *unused, DWORD kind, DWORD id, DWORD arg) {
 (void)unused;
 DWORD *ui=(DWORD *)manager[0x3b0/4];
 if(waiter_active && ui && ui[0x284/4]==7)return;
 ((void (__attribute__((thiscall)) *)(DWORD *,DWORD,DWORD,DWORD))0x5d4fa0)(manager,kind,id,arg);
}
static const struct { DWORD at; void *callback; } script_hooks[]={
 {0xcd57f0,script_frame},{0xcd5b98,script_se},{0xcd5bec,upcut_start},{0xcd5cb8,upcut_change},{0xcd5d78,sub_ready_temp},{0xcd5d84,sub_get_result}};
static DWORD typewriter_id;
typedef const char *(__attribute__((thiscall)) *lookup_text)(DWORD *,DWORD);
static DWORD *common_messages(void) {
 DWORD *manager=*(DWORD **)0xe2daa4;
 return manager && manager[0x1a4/4]?(DWORD *)manager[0x158/4]:NULL;
}
/* Typewriter commonmsg12 (ink) / 13 (no ink): show only the choice page. */
static void __attribute__((fastcall)) typewriter_message(DWORD *manager, void *unused, DWORD id, DWORD arg) {
 (void)unused;
 DWORD *messages=common_messages(), *ui=(DWORD *)manager[0x3b0/4];
 const char *text=messages?((lookup_text)0x887090)(messages,id):NULL;
 int choice=text && strstr(last_page(text),"<SEL")!=NULL;
 /* re0box hooks 0x57ada4 (jmp); its no-ink prompt then treats choice 1 as "open box". */
 int box=id==13 && *(BYTE *)0x57ada4==0xe9;
 if(text && ui && ui[0x284/4]==7 && (!choice || box)){
  if(choice)ui[0x2a8/4]=1;
  return;
 }
 typewriter_id=id;
 ((void (__attribute__((thiscall)) *)(DWORD *,DWORD,DWORD))0x5d5060)(manager,id,arg);
 typewriter_id=0;
}
static const char * __attribute__((fastcall)) typewriter_text(DWORD *messages, void *unused, DWORD id) {
 (void)unused;
 const char *text=((lookup_text)0x887090)(messages,id);
 return typewriter_id && typewriter_id==id && messages==common_messages()?last_page(text):text;
}
static void __attribute__((fastcall)) save_sound(DWORD *ui, void *unused, DWORD *text, DWORD character) {
 (void)unused;
 if((DWORD)(ULONG_PTR)ui==save_ui && ui[0]==0xce07e0 && ui[0x3e8/4]==6 && ui[0x3ec/4]==2){
  DWORD slot=ui[0x3d4/4], at=(DWORD)(ULONG_PTR)text;
  if(slot<7 && (((BYTE *)ui)[0x3cc]?ui[0x300/4+slot]==at:
      ui[0x31c/4+slot]==at || ui[0x338/4+slot]==at))
   if(!play_typing_sound(&sound_state,at,text[0x174/4],save_text_speed))return;
 }
 ((void (__attribute__((thiscall)) *)(DWORD *,DWORD *,DWORD))0x5d2890)(ui,text,character);
}
static void note(const char *s) {
 HANDLE h=CreateFileA("RE0.UISpeedTweaks.log",FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
 if(h!=INVALID_HANDLE_VALUE){DWORD written;WriteFile(h,s,(DWORD)strlen(s),&written,NULL);CloseHandle(h);}
}
static DWORD WINAPI apply(void *module) {
 wchar_t ini[32768], value[128];
 DWORD length=GetModuleFileNameW((HMODULE)module,ini,32768);
 if(length==0 || length>=32768){note("INI path unavailable; skipped.\r\n");return 0;}
 wchar_t *dot=wcsrchr(ini,L'.');
 if(!dot || dot-ini>32763){note("INI path invalid; skipped.\r\n");return 0;}
 wcscpy(dot,L".ini");
 int valid;
 GetPrivateProfileStringW(L"Timing",L"WaitSeconds",L"0.3",value,128,ini);
 wait_seconds=parse_setting(value,0,30,0.3f,&valid);
 if(!valid)note("Invalid WaitSeconds; using 0.3 seconds.\r\n");
 GetPrivateProfileStringW(L"Timing",L"FadeSpeed",L"2.0",value,128,ini);
 fade_speed=parse_setting(value,0.1,20,2,&valid);
 if(!valid)note("Invalid FadeSpeed; using 2.0.\r\n");
 GetPrivateProfileStringW(L"Timing",L"RecapTextSpeed",L"4.0",value,128,ini);
 recap_speed=parse_setting(value,0.1,20,4,&valid);
 if(!valid)note("Invalid RecapTextSpeed; using 4.0.\r\n");
 recap_interval=8.0f/recap_speed;
 GetPrivateProfileStringW(L"Timing",L"PartnerTransitionSpeed",L"2.0",value,128,ini);
 partner_speed=parse_setting(value,0.1,20,2,&valid);
 if(!valid)note("Invalid PartnerTransitionSpeed; using 2.0.\r\n");
 GetPrivateProfileStringW(L"Timing",L"SaveTextSpeed",L"4.0",value,128,ini);
 save_text_speed=parse_setting(value,0.1,20,4,&valid);
 if(!valid)note("Invalid SaveTextSpeed; using 4.0.\r\n");
 GetPrivateProfileStringW(L"Timing",L"SaveTextHoldSeconds",L"0.3",value,128,ini);
 save_text_hold=parse_setting(value,0,30,0.3f,&valid);
 if(!valid)note("Invalid SaveTextHoldSeconds; using 0.3 seconds.\r\n");
 GetPrivateProfileStringW(L"Typewriter",L"SkipTypewriterMessage",L"1",value,128,ini);
 skip_typewriter_intro=wcscmp(value,L"0")!=0;
 if(skip_typewriter_intro && wcscmp(value,L"1"))note("Invalid SkipTypewriterMessage; using 1.\r\n");
 GetPrivateProfileStringW(L"Timing",L"DumbwaiterSpeed",L"5.0",value,128,ini);
 dumbwaiter_speed=parse_setting(value,1,50,5,&valid);
 if(!valid)note("Invalid DumbwaiterSpeed; using 5.0.\r\n");
 Sleep(500);
 if((ULONG_PTR)GetModuleHandleA(NULL)!=0x400000){note("Unsupported image base; skipped.\r\n");return 0;}
 for(unsigned i=0;i<PATCH_COUNT;i++)
  if(memcmp((void *)(ULONG_PTR)patches[i].at,patches[i].before,patches[i].size)){note("Instruction guard failed; no patches applied.\r\n");return 0;}
 struct patch runtime[PATCH_COUNT];
 memcpy(runtime,patches,sizeof(runtime));
 BYTE *stubs=VirtualAlloc(NULL,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!stubs){note("Stub allocation failed; skipped.\r\n");return 0;}
 unsigned stub_count=0;
 for(unsigned i=0;i<PATCH_COUNT;i++){
  if(runtime[i].kind==WAIT || runtime[i].kind==RECAP){
   DWORD address=(DWORD)(ULONG_PTR)(runtime[i].kind==RECAP?&recap_interval:&wait_seconds);
   memcpy(runtime[i].after+4,&address,4);
  } else if(runtime[i].kind==NOTICE){
   float notice=wait_seconds>0?wait_seconds:0.001f;memcpy(runtime[i].after+3,&notice,4);
  } else if(runtime[i].kind==SAVE_SOUND){
   DWORD callback=(DWORD)(ULONG_PTR)&save_sound;memcpy(runtime[i].after,&callback,4);
  } else if(runtime[i].kind==SAVE_HOLD){
   memcpy(runtime[i].after+6,&save_text_hold,4);
  } else if(runtime[i].kind==TYPEWRITER_CALL || runtime[i].kind==TYPEWRITER_TEXT){
   if(!skip_typewriter_intro){memcpy(runtime[i].after,runtime[i].before,runtime[i].size);continue;}
   DWORD callback=(DWORD)(ULONG_PTR)(runtime[i].kind==TYPEWRITER_CALL?(void *)&typewriter_message:(void *)&typewriter_text);
   DWORD delta=callback-runtime[i].at-5;memcpy(runtime[i].after+1,&delta,4);
  } else if(runtime[i].kind==DUMBWAITER_MESSAGE){
   if(dumbwaiter_speed==1){memcpy(runtime[i].after,runtime[i].before,5);continue;}
   DWORD delta=(DWORD)(ULONG_PTR)&dumbwaiter_message-runtime[i].at-5;memcpy(runtime[i].after+1,&delta,4);
  } else if(runtime[i].kind==SCRIPT_SLOT){
   if(dumbwaiter_speed==1){memcpy(runtime[i].after,runtime[i].before,4);continue;}
   for(unsigned j=0;j<sizeof(script_hooks)/sizeof(script_hooks[0]);j++)
    if(script_hooks[j].at==runtime[i].at){DWORD callback=(DWORD)(ULONG_PTR)script_hooks[j].callback;memcpy(runtime[i].after,&callback,4);}
  } else {
   BYTE *stub=stubs+STUB_SIZE*stub_count++;
   DWORD speed=(DWORD)(ULONG_PTR)(runtime[i].kind==FADE?&fade_speed:runtime[i].kind==SAVE_TEXT?&save_text_speed:&partner_speed);
   write_stub(stub,(DWORD)(ULONG_PTR)stub,&runtime[i],speed,(DWORD)(ULONG_PTR)&save_ui);
  }
 }
 DWORD previous;
 if(!VirtualProtect(stubs,8192,PAGE_EXECUTE_READ,&previous)){VirtualFree(stubs,0,MEM_RELEASE);note("Stub protection failed; skipped.\r\n");return 0;}
 FlushInstructionCache(GetCurrentProcess(),stubs,8192);
 DWORD old[PATCH_COUNT];unsigned count=0;
 for(;count<PATCH_COUNT;count++)if(!VirtualProtect((void *)(ULONG_PTR)runtime[count].at,runtime[count].size,PAGE_EXECUTE_READWRITE,&old[count]))break;
 if(count<PATCH_COUNT){for(unsigned i=count;i>0;i--){DWORD tmp;VirtualProtect((void *)(ULONG_PTR)runtime[i-1].at,runtime[i-1].size,old[i-1],&tmp);}VirtualFree(stubs,0,MEM_RELEASE);note("Protection failed; no patches applied.\r\n");return 0;}
 for(unsigned i=0;i<PATCH_COUNT;i++)memcpy((void *)(ULONG_PTR)runtime[i].at,runtime[i].after,runtime[i].size);
 FlushInstructionCache(GetCurrentProcess(),NULL,0);
 for(unsigned i=PATCH_COUNT;i>0;i--){DWORD tmp;VirtualProtect((void *)(ULONG_PTR)runtime[i-1].at,runtime[i-1].size,old[i-1],&tmp);}
 char message[640];
 snprintf(message,sizeof(message),"Applied %u guarded patches (%u stubs); WaitSeconds=%.6g FadeSpeed=%.6g RecapTextSpeed=%.6g PartnerTransitionSpeed=%.6g SaveTextSpeed=%.6g SaveTextHoldSeconds=%.6g SkipTypewriterMessage=%d DumbwaiterSpeed=%.6g; readiness checks preserved.\r\n",(unsigned)PATCH_COUNT,stub_count,(double)wait_seconds,(double)fade_speed,(double)recap_speed,(double)partner_speed,(double)save_text_speed,(double)save_text_hold,skip_typewriter_intro,(double)dumbwaiter_speed);
 note(message);
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE dll,DWORD reason,LPVOID reserved) {
 (void)reserved;
 if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(dll);HANDLE t=CreateThread(NULL,0,apply,dll,0,NULL);if(t)CloseHandle(t);}
 return TRUE;
}
