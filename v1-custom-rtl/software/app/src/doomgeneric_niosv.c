#include "doomgeneric.h"
#include "doomkeys.h"
#include "i_video.h"
#include "i_system.h"
#include "ps2_keys.h"
#include "ramfs.h"
#include "system.h"
#include "io.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "doom_profile.h"

#ifdef DOOM_FPS_OVERLAY
    #include "fps_overlay.h"
    static FpsCounter fps_counter;
    static uint8_t fps_pixels[FPS_WIDTH*FPS_HEIGHT];
#endif

extern const uint8_t embedded_wad[];
extern const unsigned int embedded_wad_size;
extern const char embedded_wad_name[];

void niosv_fs_init(void);

#define REG_STATUS 0x21000
#define REG_SUBMIT 0x21004
#define REG_MS 0x21008
#define REG_KEY 0x2100c
#define REG_ERROR 0x21010

static Ps2Parser parser;
static uint8_t held[256];
static int recovering, release_index;
static uint32_t frames;

void DG_Init(void) {
	if (IORD_32DIRECT(IO_BASE, 0x21014) != 0x444f4f4d)
		I_Error("DOOM IO hardware ID mismatch");
	puts("DOOM Nios V/g: silent, indexed VGA, PS/2, RAM files");
}

uint32_t DG_GetTicksMs(void) {
	return IORD_32DIRECT(IO_BASE, REG_MS);
}

void DG_SleepMs(uint32_t ms) {
	uint32_t start = DG_GetTicksMs();
	while ((uint32_t)(DG_GetTicksMs() - start) < ms) {
	}
}

int DG_GetKey(int *pressed, unsigned char *key) {
	if (DP_IS_BENCHMARK)
		return 0;
	uint32_t raw, err;
	err = IORD_32DIRECT(IO_BASE, REG_ERROR);

	if (err) {
		IOWR_32DIRECT(IO_BASE, REG_ERROR, err);
		parser = (Ps2Parser ) { 0 };
		recovering = 1;
		release_index = 0;
		while (IORD_32DIRECT(IO_BASE, REG_KEY) & 256) {
		}
	}

	if (recovering) {
		while (release_index < 256) {
			int k = release_index++;
			if (held[k]) {
				held[k] = 0;
				*key = k;
				*pressed = 0;
				return 1;
			}
		}
		recovering = 0;
	}

	while ((raw = IORD_32DIRECT(IO_BASE, REG_KEY)) & 256) {
		if (ps2_key(&parser, (uint8_t) raw, pressed, key)) {
			if (held[*key] == *pressed)
				continue;
			held[*key] = *pressed;
			return 1;
		}
	}
	return 0;
}

void DG_DrawFrame(void) {
	uint32_t status, start = DG_GetTicksMs(), bank;
	unsigned i;
	DP_Begin(DP_WAIT);

	do {
		status = IORD_32DIRECT(IO_BASE, REG_STATUS);
		if ((uint32_t)(DG_GetTicksMs() - start) > 1000)
			I_Error("VGA swap timeout");
	} while (status & 2);

	DP_End(DP_WAIT);
	DP_Begin(DP_TRANSFER);
	bank = (status & 1) ^ 1;

	for (i = 0; i < 16000; i++) {
		const uint8_t *p = DG_ScreenBuffer + 4 * i;
		uint32_t v = (uint32_t) p[0] | ((uint32_t) p[1] << 8)
				| ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
		IOWR_32DIRECT(IO_BASE, bank * 0x10000 + 4 * i, v);
	}

#ifdef DOOM_FPS_OVERLAY
    // Paint only the outgoing VGA bank, leaving engine and DG buffers untouched.
    { unsigned lo=0,hi=0,j,x,y; unsigned low=766,high=0;
      for(j=0;j<256;j++) { unsigned l=colors[j].r+colors[j].g+colors[j].b;
        if(l<low){low=l;lo=j;} if(l>high){high=l;hi=j;} }
      fps_bitmap(&fps_counter,fps_pixels);
      for(y=0;y<FPS_HEIGHT;y++)for(x=0;x<FPS_WIDTH;x+=4) {
        uint32_t value=0;
        for(j=0;j<4;j++)value|=(uint32_t)(fps_pixels[y*FPS_WIDTH+x+j]?hi:lo)<<(8*j);
        IOWR_32DIRECT(IO_BASE,bank*0x10000+(FPS_Y+y)*320+FPS_X+x,value);
      }
    }
#endif

	for (i = 0; i < 256; i++)
		IOWR_32DIRECT(IO_BASE, 0x20000 + bank * 0x400 + 4 * i,
				(colors[i].r << 16) | (colors[i].g << 8) | colors[i].b);
	__asm__ volatile("fence iorw,iorw" ::: "memory");
	IOWR_32DIRECT(IO_BASE, REG_SUBMIT, bank | 2);
	frames++;

#ifdef DOOM_FPS_OVERLAY
    fps_sample(&fps_counter,DG_GetTicksMs());
#endif
	DP_End(DP_TRANSFER);
	if (!DP_IS_BENCHMARK && (frames == 1 || frames % 120 == 0)) {
		clearerr(stdout);
		printf("VGA frames=%lu uptime_ms=%lu\n", (unsigned long) frames,
				(unsigned long) DG_GetTicksMs());
	}
}
void DG_SetWindowTitle(const char *title) {
	(void) title;
}
int main(void) {
#ifdef DOOM_BENCHMARK
    extern const uint8_t embedded_demo[];
    extern const unsigned int embedded_demo_size;
    char path[128];char *argv[]={"doom","-iwad",path,"-nosound","-mb","16","-timedemo","/ram/bench.lmp",NULL};
#else
	char path[128];
	char *argv[] = { "doom", "-iwad", path, "-nosound", "-mb", "16", NULL };
#endif
	setvbuf(stdout, NULL, _IONBF, 0);
	niosv_fs_init();
	snprintf(path, sizeof(path), "/ram/%s", embedded_wad_name);
	// Gameplay must continue when no JTAG console is connected.
	fcntl(1, F_SETFL, O_NONBLOCK);
	fcntl(2, F_SETFL, O_NONBLOCK);
	if (ramfs_mount_wad(path, embedded_wad, embedded_wad_size) < 0) {
		puts("Cannot mount WAD");
		return 1;
	}
#ifdef DOOM_BENCHMARK
    if(ramfs_mount_wad("/ram/bench.lmp",embedded_demo,embedded_demo_size)<0){puts("Cannot mount benchmark");return 1;}
    doomgeneric_Create(8,argv);
#else
	doomgeneric_Create(6, argv);
#endif
	for (;;)
		doomgeneric_Tick();
}
