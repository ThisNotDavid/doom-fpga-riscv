#ifndef FPS_OVERLAY_H
#define FPS_OVERLAY_H
#include <stdint.h>
#define FPS_WIDTH 88
#define FPS_HEIGHT 14
#define FPS_X (320-FPS_WIDTH-4)
#define FPS_Y 2
typedef struct { uint32_t start, frames, tenths; int started, valid; } FpsCounter;
static void fps_sample(FpsCounter *c, uint32_t now) {
    uint32_t elapsed;
    if (!c->started) { c->started=1; c->start=now; return; }
    c->frames++;
    elapsed=now-c->start;
    if (elapsed>=1000) {
        c->tenths=(uint32_t)(((uint64_t)c->frames*10000+elapsed/2)/elapsed);
        if(c->tenths>9999)c->tenths=9999;
        c->valid=1; c->frames=0; c->start=now;
    }
}
static unsigned fps_glyph(char ch) {
    static const unsigned digits[10]={075557,026227,071747,071717,055711,074717,074757,071111,075757,075717};
    if(ch>='0' && ch<='9')return digits[ch-'0'];
    switch(ch) { case 'F':return 074744; case 'P':return 075744;
        case 'S':return 074717; case ':':return 002020; case '.':return 000002;
        case '-':return 000700; default:return 0; }
}
static void fps_bitmap(const FpsCounter *c, uint8_t *bitmap) {
    char text[12]="FPS: --"; unsigned n=5,i,x,y,bit;
    for(i=0;i<FPS_WIDTH*FPS_HEIGHT;i++)bitmap[i]=0;
    if(c->valid) {
        unsigned v=c->tenths/10;
        if(v>=100)text[n++]=(char)('0'+v/100);
        if(v>=10)text[n++]=(char)('0'+v/10%10);
        text[n++]=(char)('0'+v%10);text[n++]='.';
        text[n++]=(char)('0'+c->tenths%10);text[n]=0;
    }
    for(i=0;text[i];i++) {
        unsigned glyph=fps_glyph(text[i]);
        for(y=0;y<5;y++)for(x=0;x<3;x++) {
            bit=(glyph>>((4-y)*3+2-x))&1;
            bitmap[(2+2*y)*FPS_WIDTH+2+i*8+2*x]=(uint8_t)bit;
            bitmap[(2+2*y)*FPS_WIDTH+3+i*8+2*x]=(uint8_t)bit;
            bitmap[(3+2*y)*FPS_WIDTH+2+i*8+2*x]=(uint8_t)bit;
            bitmap[(3+2*y)*FPS_WIDTH+3+i*8+2*x]=(uint8_t)bit;
        }
    }
}
#endif
