#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <errno.h>
static float parse_setting(const wchar_t *text, double low, double high, float fallback, int *valid) {
 wchar_t *end;
 errno=0;
 double value=wcstod(text,&end);
 *valid=0;
 if(end==text || errno==ERANGE || !isfinite(value) || value<low || value>high)return fallback;
 while(iswspace(*end))end++;
 if(*end)return fallback;
 *valid=1;
 return (float)value;
}
