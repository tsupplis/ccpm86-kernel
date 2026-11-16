/* Decode independently of the native console and command controller. */
#include <assert.h>
#include <stdio.h>
#include "fm.h"
int main(void)
{
    int index;
    for (index = 0; index < 10; ++index) {
        assert(fm_key_decode(0xe1 + index) == FM_KEY_F1 + index);
        assert(fm_key_decode(0xbb + index) == FM_KEY_F1 + index);
    }
    assert(fm_key_decode(1) == FM_KEY_HOME);
    assert(fm_key_decode(26) == FM_KEY_END);
    assert(fm_key_decode(3) == FM_KEY_TAB);
    assert(fm_key_decode(11) == FM_KEY_UP);
    assert(fm_key_decode(10) == FM_KEY_DOWN);
    assert(fm_key_decode(12) == FM_KEY_RIGHT);
    assert(fm_key_decode(8) == 8);
    assert(fm_key_decode(127) == FM_KEY_DELETE);
    assert(fm_key_decode('a') == 'a');
    assert(fm_key_decode(FM_KEY_TICK) == FM_KEY_TICK);
    puts("PASS: PC key ranges, historical control aliases and literal preservation");
    return 0;
}
