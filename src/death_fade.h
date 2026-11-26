/* Game-over white fade: keep the native timer and completion, zero the alpha of the
   current (+0x274), start (+0x278) and target (+0x27c) ARGB colours so nothing is drawn. */
static void hide_fade(unsigned char *fade) {
 fade[0x277]=fade[0x27b]=fade[0x27f]=0;
}
