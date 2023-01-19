# cobs
Consistent Overhead Byte Stuffing

The algorithm is described at [Wikipedia](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing).

## Example

```c
#include <assert.h>
#include <libcobs/cobs.h>
#include <string.h>

int main(void)
{
    uint8_t plaintext[] = { 0, 10, 0, 20, 0, 0, 30, 0 };
    uint8_t stuffed[9];

    ssize_t length = cobs_encode(plaintext, sizeof(plaintext), stuffed, sizeof(stuffed));
    assert(length == 9);

    assert(stuffed[0] == 1);
    assert(stuffed[1] == 2);
    assert(stuffed[2] == 10);
    assert(stuffed[3] == 2);
    assert(stuffed[4] == 20);
    assert(stuffed[5] == 1);
    assert(stuffed[6] == 2);
    assert(stuffed[7] == 30);
    assert(stuffed[8] == 1);

    uint8_t roundtrip[8];
    length = cobs_decode(stuffed, sizeof(stuffed), roundtrip, sizeof(roundtrip), true);
    assert(length == 8);
    assert(memcmp(plaintext, roundtrip, sizeof(plaintext)) == 0);
}
```
