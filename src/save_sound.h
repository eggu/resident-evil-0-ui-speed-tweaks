#include <stdint.h>
struct typing_sound { uint32_t text, count; float credit; };
static int play_typing_sound(struct typing_sound *state, uint32_t text, uint32_t count, float speed) {
 int first=state->text!=text || count<=state->count;
 state->text=text;state->count=count;
 if(first || speed<=1){state->credit=0;return 1;}
 state->credit+=1;
 if(state->credit<speed)return 0;
 state->credit-=speed;
 return 1;
}
