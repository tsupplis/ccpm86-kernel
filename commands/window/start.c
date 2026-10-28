#include "win.h"
/* DRC 1.11's small-model startup calls _main. */
_main() { main(); bdos(0,0); }
