#include "ps2_keys.h"
#include "doomkeys.h"

int ps2_key(Ps2Parser *p,uint8_t b,int *pressed,uint8_t *key) {
    static const uint8_t ascii[128]={
        [0x1c]='a',[0x32]='b',[0x21]='c',[0x23]='d',[0x24]='e',[0x2b]='f',[0x34]='g',
        [0x33]='h',[0x43]='i',[0x3b]='j',[0x42]='k',[0x4b]='l',[0x3a]='m',[0x31]='n',
        [0x44]='o',[0x4d]='p',[0x15]='q',[0x2d]='r',[0x1b]='s',[0x2c]='t',[0x3c]='u',
        [0x2a]='v',[0x1d]='w',[0x22]='x',[0x35]='y',[0x1a]='z',
        [0x16]='1',[0x1e]='2',[0x26]='3',[0x25]='4',[0x2e]='5',[0x36]='6',
        [0x3d]='7',[0x3e]='8',[0x46]='9',[0x45]='0',[0x29]=' ',[0x4e]='-',
        [0x55]='=',[0x41]=',',[0x49]='.',[0x4a]='/',[0x4c]=';',[0x52]='\'',
        [0x54]='[',[0x5b]=']',[0x5d]='\\',[0x0e]='`',

        [0x5a]=KEY_ENTER,[0x76]=KEY_ESCAPE,[0x66]=KEY_BACKSPACE,[0x0d]=KEY_TAB,
        [0x12]=KEY_RSHIFT,[0x59]=KEY_RSHIFT,[0x14]=KEY_RCTRL,[0x11]=KEY_RALT,
        [0x05]=KEY_F1,[0x06]=KEY_F2,[0x04]=KEY_F3,[0x0c]=KEY_F4,
        [0x03]=KEY_F5,[0x0b]=KEY_F6,[0x0a]=KEY_F8,[0x01]=KEY_F9,
        [0x09]=KEY_F10,[0x78]=KEY_F11,[0x07]=KEY_F12};

    uint8_t k = 0;

	if (p->pause_skip) {
		--p->pause_skip;
		return 0;
	}

	if (b == 0xe1) {
		p->pause_skip = 7;
		p->extended = p->released = 0;
		return 0;
	}

	if (b == 0xe0) {
		p->extended = 1;
		return 0;
	}

	if (b == 0xf0) {
		p->released = 1;
		return 0;
	}

	if (p->extended)
		switch (b) {
		case 0x75:
			k = KEY_UPARROW;
			break;
		case 0x72:
			k = KEY_DOWNARROW;
			break;
		case 0x6b:
			k = KEY_LEFTARROW;
			break;
		case 0x74:
			k = KEY_RIGHTARROW;
			break;
		case 0x14:
			k = KEY_RCTRL;
			break;
		case 0x11:
			k = KEY_RALT;
			break;
		case 0x5a:
			k = KEY_ENTER;
			break;
		default:
			break;
		}
	else if (b == 0x83)
		k = KEY_F7;
	else if (b < 128)
		k = ascii[b];
	*pressed = !p->released;
	*key = k;
	p->extended = p->released = 0;
	return k != 0;
}

