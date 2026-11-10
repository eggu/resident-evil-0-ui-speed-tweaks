#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "save_sound.h"
#include "patches.h"
#include "stubs.h"
_Static_assert(PATCH_COUNT*STUB_SIZE<=4096,"Stub page too small");
static float wait_seconds=0.3f, fade_speed=2.0f, recap_speed=4.0f, recap_interval=2.0f, partner_speed=2.0f, save_text_speed=4.0f, save_text_hold=0.3f;
static DWORD save_ui;
static struct typing_sound sound_state;
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
 Sleep(500);
 if((ULONG_PTR)GetModuleHandleA(NULL)!=0x400000){note("Unsupported image base; skipped.\r\n");return 0;}
 for(unsigned i=0;i<PATCH_COUNT;i++)
  if(memcmp((void *)(ULONG_PTR)patches[i].at,patches[i].before,patches[i].size)){note("Instruction guard failed; no patches applied.\r\n");return 0;}
 struct patch runtime[PATCH_COUNT];
 memcpy(runtime,patches,sizeof(runtime));
 BYTE *stubs=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
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
  } else {
   BYTE *stub=stubs+STUB_SIZE*stub_count++;
   DWORD speed=(DWORD)(ULONG_PTR)(runtime[i].kind==FADE?&fade_speed:runtime[i].kind==SAVE_TEXT?&save_text_speed:&partner_speed);
   write_stub(stub,(DWORD)(ULONG_PTR)stub,&runtime[i],speed,(DWORD)(ULONG_PTR)&save_ui);
  }
 }
 DWORD previous;
 if(!VirtualProtect(stubs,4096,PAGE_EXECUTE_READ,&previous)){VirtualFree(stubs,0,MEM_RELEASE);note("Stub protection failed; skipped.\r\n");return 0;}
 FlushInstructionCache(GetCurrentProcess(),stubs,4096);
 DWORD old[PATCH_COUNT];unsigned count=0;
 for(;count<PATCH_COUNT;count++)if(!VirtualProtect((void *)(ULONG_PTR)runtime[count].at,runtime[count].size,PAGE_EXECUTE_READWRITE,&old[count]))break;
 if(count<PATCH_COUNT){for(unsigned i=count;i>0;i--){DWORD tmp;VirtualProtect((void *)(ULONG_PTR)runtime[i-1].at,runtime[i-1].size,old[i-1],&tmp);}VirtualFree(stubs,0,MEM_RELEASE);note("Protection failed; no patches applied.\r\n");return 0;}
 for(unsigned i=0;i<PATCH_COUNT;i++)memcpy((void *)(ULONG_PTR)runtime[i].at,runtime[i].after,runtime[i].size);
 FlushInstructionCache(GetCurrentProcess(),NULL,0);
 for(unsigned i=PATCH_COUNT;i>0;i--){DWORD tmp;VirtualProtect((void *)(ULONG_PTR)runtime[i-1].at,runtime[i-1].size,old[i-1],&tmp);}
 char message[480];
 snprintf(message,sizeof(message),"Applied %u guarded patches (%u stubs); WaitSeconds=%.6g FadeSpeed=%.6g RecapTextSpeed=%.6g PartnerTransitionSpeed=%.6g SaveTextSpeed=%.6g SaveTextHoldSeconds=%.6g; readiness checks preserved.\r\n",(unsigned)PATCH_COUNT,stub_count,(double)wait_seconds,(double)fade_speed,(double)recap_speed,(double)partner_speed,(double)save_text_speed,(double)save_text_hold);
 note(message);
 return 0;
}
BOOL WINAPI DllMain(HINSTANCE dll,DWORD reason,LPVOID reserved) {
 (void)reserved;
 if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(dll);HANDLE t=CreateThread(NULL,0,apply,dll,0,NULL);if(t)CloseHandle(t);}
 return TRUE;
}
