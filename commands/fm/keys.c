/* Raw PC-console key decoding. Pure: no I/O, UI model or global state.
 * Both XIOS function-key ranges are accepted. Control-key aliases follow
 * original FM CODE 68C8..6909. Backspace retains the browser's parent action;
 * serial TERM.DAT translation belongs to a future terminal adapter.
 */
#include "fm.h"
int fm_key_decode(int key)
{
    if (key >= 0xe1 && key <= 0xea)
        return FM_KEY_F1 + key - 0xe1;
    if (key >= 0xbb && key <= 0xc4)
        return FM_KEY_F1 + key - 0xbb;
    switch (key) {
    case 1:
    case 0xc7:
        return FM_KEY_HOME;
    case 26:
    case 0xcf:
        return FM_KEY_END;
    case 3:
        return FM_KEY_TAB;
    case 11:
    case 0xc8:
        return FM_KEY_UP;
    case 10:
    case 0xd0:
        return FM_KEY_DOWN;
    case 12:
    case 0xcd:
        return FM_KEY_RIGHT;
    case 0xcb:
        return FM_KEY_LEFT;
    case 0xc9:
        return FM_KEY_PGUP;
    case 0xd1:
        return FM_KEY_PGDN;
    case 0xd2:
        return FM_KEY_INSERT;
    case 127:
    case 0xd3:
        return FM_KEY_DELETE;
    default:
        return key;
    }
}
