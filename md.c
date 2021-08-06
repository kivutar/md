#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "u.h"
#include "compat.h"
#include "dat.h"
#include "fns.h"

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;
static retro_video_refresh_t video_cb;
static retro_environment_t environ_cb;
retro_audio_sample_batch_t audio_cb;

int t;
u16int *prg;
int nprg;
u8int *sram;
u32int sramctl, nsram, sram0, sram1;
int savefd = -1;
int doflush = 0;
u64int keys, keys2;
uchar *pic;

int dmaclock, vdpclock, z80clock, audioclock, ymclock, saveclock;

void
flushram(void)
{
	if(savefd >= 0)
		pwrite(savefd, sram, nsram, 0);
	saveclock = 0;
}

static void
loadbat(const char *file)
{
	static char buf[512];
	
	strncpy(buf, file, sizeof buf - 5);
	strcat(buf, ".sav");
	// savefd = create(buf, ORDWR | OEXCL);
	// if(savefd < 0)
	// 	savefd = open(buf, ORDWR);
	// if(savefd < 0)
	// 	print("open: %r\n");
	// else
	// 	readn(savefd, sram, nsram);
}

static void
loadrom(const char *file)
{
	static uchar hdr[512], buf[4096];
	u32int v;
	u16int *p;
	int fd, rc, i;
	
	fd = open(file, OREAD);
	if(fd < 0)
		sysfatal("open\n");
	if(readn(fd, hdr, 512) < 512)
		sysfatal("read\n");
	if(memcmp(hdr + 0x100, "SEGA MEGA DRIVE ", 16) != 0 && memcmp(hdr + 0x100, "SEGA GENESIS    ", 16) != 0)
		sysfatal("invalid rom\n");
	v = hdr[0x1a0] << 24 | hdr[0x1a1] << 16 | hdr[0x1a2] << 8 | hdr[0x1a3];
	if(v != 0)
		sysfatal("rom starts at nonzero address\n");
	v = hdr[0x1a4] << 24 | hdr[0x1a5] << 16 | hdr[0x1a6] << 8 | hdr[0x1a7];
	nprg = v = v+2 & ~1;
	if(nprg == 0)
		sysfatal("invalid rom\n");
	p = prg = malloc(v);
	if(prg == nil)
		sysfatal("malloc\n");
	seek(fd, 0, 0);
	while(v != 0){
		rc = readn(fd, buf, sizeof buf);
		if(rc == 0)
			break;
		if(rc < 0)
			sysfatal("read\n");
		if(rc > v)
			rc = v;
		for(i = 0; i < rc; i += 2)
			*p++ = buf[i] << 8 | buf[i+1];
		v -= rc;
	}
	close(fd);
	if(hdr[0x1b0] == 0x52 && hdr[0x1b1] == 0x41){
		sramctl = SRAM | hdr[0x1b2] >> 1 & ADDRMASK;
		if((hdr[0x1b2] & 0x40) != 0)
			sramctl |= BATTERY;
		sram0 = hdr[0x1b4] << 24 | hdr[0x1b5] << 16 | hdr[0x1b6] << 8 | hdr[0x1b7] & 0xfe;
		sram1 = hdr[0x1b8] << 24 | hdr[0x1b9] << 16 | hdr[0x1ba] << 8 | hdr[0x1bb] | 1;
		if(sram1 <= sram0){
			print("SRAM of size <= 0?\n");
			sramctl = 0;
		}else{
			nsram = sram1 - sram0;
			if((sramctl & ADDRMASK) != ADDRBOTH)
				nsram >>= 1;
			sram = malloc(nsram);
			if(sram == nil)
				sysfatal("malloc\n");
			if((sramctl & BATTERY) != 0){
				loadbat(file);
				atexit(flushram);
			}
		}
	}
}

void
retro_init(void)
{
}

void
retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "md";
	info->library_version = "1.0";
	info->need_fullpath = true;
	info->valid_extensions = "md|bin";
}

void
retro_get_system_av_info(struct retro_system_av_info *info)
{
	info->timing.fps = 60.0;
	info->timing.sample_rate = RATE;

	info->geometry.base_width = 320;
	info->geometry.base_height = 224;
	info->geometry.max_width = 320;
	info->geometry.max_height = 224;
	info->geometry.aspect_ratio = 4.0 / 3.0;
}

unsigned
retro_api_version(void)
{
	return RETRO_API_VERSION;
}

bool
retro_load_game(const struct retro_game_info *game)
{
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
		return false;

	pic = malloc(320 * 224 * 4);
	initaudio();
	loadrom(game->path);
	cpureset();
	vdpmode();
	ymreset();
	return true;
}

static const int map[] = {
	[RETRO_DEVICE_ID_JOYPAD_B] = 1<<5, // BUTTON_A
	[RETRO_DEVICE_ID_JOYPAD_Y] = 0, // BUTTON_X
	[RETRO_DEVICE_ID_JOYPAD_SELECT] = 0, // BUTTON_MODE
	[RETRO_DEVICE_ID_JOYPAD_START] = 1<<13, // BUTTON_START
	[RETRO_DEVICE_ID_JOYPAD_UP] = 0x101, // DPAD_UP
	[RETRO_DEVICE_ID_JOYPAD_DOWN] = 0x202, // DPAD_DOWN
	[RETRO_DEVICE_ID_JOYPAD_LEFT] = 1<<2, // DPAD_LEFT
	[RETRO_DEVICE_ID_JOYPAD_RIGHT] = 1<<3, // DPAD_RIGHT
	[RETRO_DEVICE_ID_JOYPAD_A] = 1<<4, // BUTTON_B
	[RETRO_DEVICE_ID_JOYPAD_X] = 1<<12, // BUTTON_Y
	[RETRO_DEVICE_ID_JOYPAD_L] = 0, // BUTTON_Z
	[RETRO_DEVICE_ID_JOYPAD_R] = 0 // BUTTON_C
};

void
process_inputs()
{
	keys = 0;
	for(int id = RETRO_DEVICE_ID_JOYPAD_B; id < RETRO_DEVICE_ID_JOYPAD_L2; id++)
		if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, id))
			keys ^= map[id];
}

void
retro_run(void)
{
	input_poll_cb();
	process_inputs();

	while(!doflush){
		if(dma != 1){
			t = step() * CPUDIV;
			if(dma != 0)
				dmastep();
		}else{
			t = CPUDIV;
			dmastep();
		}
		z80clock -= t;
		vdpclock -= t;
		audioclock += t;
		ymclock += t;

		while(vdpclock < 0){
			vdpstep();
			vdpclock += 8;
		}
		while(z80clock < 0)
			z80clock += z80step() * Z80DIV;
		while(audioclock >= SAMPDIV){
			audiosample();
			audioclock -= SAMPDIV;
		}
		while(ymclock >= YMDIV){
			ymstep();
			ymclock -= YMDIV;
		}
		if(saveclock > 0){
			saveclock -= t;
			if(saveclock <= 0){
				saveclock = 0;
				flushram();
			}
		}
	}
	video_cb(pic, 320, 224, 320*4);
	audioout();
	doflush = 0;
}

void
flush(void)
{
	doflush = 1;
}

void
retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void
retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void
retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void
retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;
}

void
retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_cb = cb;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {}
size_t retro_get_memory_size(unsigned id) { return 0; }
void * retro_get_memory_data(unsigned id) { return NULL; }
void retro_reset(void) {}
void retro_unload_game(void) {}
void retro_deinit(void) {}
void retro_set_audio_sample(retro_audio_sample_t cb) {}
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void *data, size_t size) { return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
unsigned retro_get_region(void) { return 0; }
