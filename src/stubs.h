#include <stdint.h>
#include <string.h>
/* Addresses are explicitly 32-bit so host checks emit the actual x86 stubs. */
static size_t write_stub(BYTE *stub, uint32_t address, struct patch *patch, uint32_t speed) {
 size_t used=0;
 BYTE scale[]={0xf3,0x0f,0x59,0x05,0,0,0,0};
 memcpy(scale+4,&speed,4);
 if(patch->kind==FADE){
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
