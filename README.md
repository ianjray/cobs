# cobs
Consistent Overhead Byte Stuffing

## Example

```
uint8_t plaintext[] = { 0, 10, 0, 20, 0, 30, 0, 40 };
uint8_t stuffed[9];
ssize_t length = cobs_encode(plaintext, sizeof(plaintext), stuffed, sizeof(stuffed));
// length  = 9
// stuffed = { 1, 2, 10, 2, 20, 2, 30, 2, 40 };
```
