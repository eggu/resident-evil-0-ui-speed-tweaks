#include <stdint.h>
#include <string.h>
#define STUB_SIZE 128
/* Addresses are explicitly 32-bit so host checks emit the actual x86 stubs. */
static size_t write_stub(BYTE *stub, uint32_t address, struct patch *patch, uint32_t speed, uint32_t save_ui) {
 size_t used=0;
 BYTE scale[]={0xf3,0x0f,0x59,0x05,0,0,0,0};
 memcpy(scale+4,&speed,4);
 if(patch->kind==SAVE_CAPTURE){
  stub[0]=0x89;stub[1]=0x35;memcpy(stub+2,&save_ui,4);used=6;
  stub[used++]=0xe8;
  uint32_t call=0x5efb50-(address+(uint32_t)used+4);
  memcpy(stub+used,&call,4);used+=4;
 } else if(patch->kind==SAVE_RELEASE){
  BYTE clear[]={0x9c,0x39,0x35,0,0,0,0,0x75,0x0a,0xc7,0x05,0,0,0,0,0,0,0,0,0x9d};
  memcpy(clear+3,&save_ui,4);memcpy(clear+11,&save_ui,4);
  memcpy(stub,clear,sizeof(clear));used=sizeof(clear);
  memcpy(stub+used,patch->before,patch->size);used+=patch->size;
 } else if(patch->kind==SAVE_TEXT){
  /* Only the active save-result slot is scaled; generic text keeps its delta. */
  BYTE filter[]={
   0x9c,0x50,0x52,0xa1,0,0,0,0,               /* save flags/eax/edx; load save UI */
   0x85,0xc0,0x74,0,                           /* null */
   0x81,0x38,0xe0,0x07,0xce,0,0x75,0,          /* uGUISave vtable */
   0x83,0xb8,0xe8,3,0,0,6,0x75,0,             /* save-result state */
   0x83,0xb8,0xec,3,0,0,2,0x75,0,             /* typing phase */
   0x8b,0x90,0xd4,3,0,0,0x83,0xfa,6,0x77,0,   /* visible slot 0..6 */
   0x80,0xb8,0xcc,3,0,0,0,0x74,0,             /* combined vs two-line */
   0x39,0xb4,0x90,0,3,0,0,0x74,0,0xeb,0,       /* combined text */
   0x39,0xb4,0x90,0x1c,3,0,0,0x74,0,           /* first line */
   0x39,0xb4,0x90,0x38,3,0,0,0x75,0,           /* second line */
   0xf3,0x0f,0x59,0x0d,0,0,0,0,                /* mulss xmm1, speed */
   0x5a,0x58,0x9d};
  memcpy(filter+4,&save_ui,4);
  /* Short branches target the shared restore, two-line check, or scale. */
  const unsigned restore=sizeof(filter)-3, two_line=69, multiply=87;
  const unsigned skips[]={11,19,28,37,48,68,86};
  for(unsigned i=0;i<sizeof(skips)/sizeof(skips[0]);i++)filter[skips[i]]=(BYTE)(restore-skips[i]-1);
  filter[57]=(BYTE)(two_line-58);filter[66]=(BYTE)(multiply-67);filter[77]=(BYTE)(multiply-78);
  memcpy(filter+91,&speed,4);
  memcpy(stub,patch->before,patch->size);used=patch->size;
  memcpy(stub+used,filter,sizeof(filter));used+=sizeof(filter);
 } else if(patch->kind==FADE){
  memcpy(stub,patch->before,6);used=6;
  scale[2]=0x5e;
  scale[3]=(BYTE)((patch->before[3]&0x38)|5);
  memcpy(stub+used,scale,8);used+=8;
  /* The relative-color variant reloads the argument later. */
  memcpy(stub+used,patch->before,6);stub[used+2]=0x11;used+=6;
 } else if(patch->kind==PARTNER_HOLD){
  const BYTE load[]={0xf3,0x0f,0x10,0x48,0x68};
  const BYTE subtract[]={0xf3,0x0f,0x5c,0xc1};
  memcpy(stub,load,5);used=5;scale[3]=0x0d;
  memcpy(stub+used,scale,8);used+=8;
  memcpy(stub+used,subtract,4);used+=4;
 } else {
  memcpy(stub,patch->before,patch->size);used=patch->size;
  if(patch->kind==PARTNER_DECAY1)scale[3]=0x0d;
  memcpy(stub+used,scale,8);used+=8;
 }
 stub[used++]=0xe9;
 uint32_t delta=patch->at+(uint32_t)patch->size-(address+(uint32_t)used+4);
 memcpy(stub+used,&delta,4);used+=4;
 patch->after[0]=0xe9;delta=address-patch->at-5;
 memcpy(patch->after+1,&delta,4);
 if(patch->size==6)patch->after[5]=0x90;
 return used;
}
