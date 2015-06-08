#include <stdint.h>
#include "../draw.h"

void main()
{
    draw_init((uint32_t *)0x23FFFE00);
    print("Hello arm9!");
}
