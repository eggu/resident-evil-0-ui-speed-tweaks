#include <string.h>
/* Text after the last <PAGE>: the choice page, or the whole text if single-page. */
static const char *last_page(const char *text) {
 const char *page;
 while((page=strstr(text,"<PAGE>")))text=page+6;
 return text;
}
