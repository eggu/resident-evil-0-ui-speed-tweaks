"""Instruction guards for the supported 32-bit Steam build."""
import struct
COUNTDOWNS=[0x5d1406,0x5d1694,0x5d194d,0x5d1a24,0x5d1d51,0x5c7de7,0x5ea920,0x5ea97e,0x5eab19]
NOTICES=[0x5d1638,0x5d174e,0x5d1a0a,0x5d1fe0]
FADE_ENTRIES=[(0x5aa2c0,'f30f10442404'),(0x5aa350,'f30f104c2404'),(0x5aa410,'f30f10442404'),(0x5aa4a0,'f30f10442404'),(0x5aa550,'f30f10442408'),(0x5aa5e0,'f30f10442404'),(0x5aa670,'f30f10442404')]
RECAP_AT=0x5becb7
PARTNER_ENTRIES=[(0x599d6f,'f30f104068','PARTNER_TICK'),(0x599dd4,'f30f5c4068','PARTNER_HOLD'),(0x599e11,'f30f594e44','PARTNER_DECAY1'),(0x599e3e,'f30f59467c','PARTNER_DECAY0')]
# Replacement operands/branches are completed with DLL addresses at startup.
PATCHES=[(a,bytes.fromhex('f30f590518a4cb00'),bytes.fromhex('f30f5905e0b5cb00')) for a in COUNTDOWNS]
PATCHES += [(a,bytes.fromhex('c7042400004040'),bytes.fromhex('c70424')+struct.pack('<f',0.3)) for a in NOTICES]
PATCHES += [(a,bytes.fromhex(signature),b'\xe9'+bytes(4)+b'\x90') for a,signature in FADE_ENTRIES]
PATCHES += [(RECAP_AT,bytes.fromhex('f30f100554dfcb00'),bytes.fromhex('f30f100554dfcb00'))]
PATCHES += [(a,bytes.fromhex(signature),b'\xe9'+bytes(4)) for a,signature,kind in PARTNER_ENTRIES]
def kind(address):
 if address==RECAP_AT:return 'RECAP'
 if address in COUNTDOWNS:return 'WAIT'
 if address in NOTICES:return 'NOTICE'
 return next((k for a,_,k in PARTNER_ENTRIES if a==address),'FADE')
def header():
 rows=['{0x%x,{%s},{%s},%d,%s}'%(a,','.join(str(x) for x in old),','.join(str(x) for x in new),len(old),kind(a)) for a,old,new in PATCHES]
 return 'enum patch_kind { WAIT, NOTICE, FADE, RECAP, PARTNER_TICK, PARTNER_HOLD, PARTNER_DECAY1, PARTNER_DECAY0 };\nstruct patch { DWORD at; BYTE before[8], after[8]; SIZE_T size; enum patch_kind kind; };\nstatic const struct patch patches[]={'+','.join(rows)+'};\nenum { PATCH_COUNT=sizeof(patches)/sizeof(patches[0]) };\n'
