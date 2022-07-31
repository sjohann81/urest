#include <stdio.h>
#include <stdint.h>

int base32_encode(char *in, uint16_t len, char *out)
{
	char alphabet[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"};
	int outlen = 0, count = 0, bufsize = 8, index = 0;
	int buffer, next, left, pad;

	if (len > 0) {
		buffer = in[0];
		next = 1;
		left = 8;

		while (count < bufsize && (left > 0 || next < len)) {
			if (left < 5) {
				if (next < len) {
					buffer <<= 8;
					buffer |= in[next] & 0xff;
					next++;
					left += 8;
				} else {
					pad = 5 - left;
					buffer <<= pad;
					left += pad;
				}
			}
			index = 0x1f & (buffer >> (left - 5));

			left -= 5;
			out[outlen] = alphabet[index];
			outlen++;
		}
		out[outlen] = '\0';
	}
  
	return outlen;
}

int base32_decode(char *in, uint16_t len, char *out)
{
	int i, outlen = 0, buffer = 0, left = 0;
	char ch;

	for (i = 0; i < len; i++) {
		ch = in[i];

		if (ch == 0xa0 || ch == 0x09 || ch == 0x0a || ch == 0x0d || ch == 0x3d)
			continue;

		if ((ch >= 0x41 && ch <= 0x5a) || (ch >= 0x61 && ch <= 0x7a)) {
			ch = ((ch & 0x1f) - 1);
		} else if (ch >= 0x32 && ch <= 0x37) {
			ch -= (0x32 - 26);
		} else {
			return 0;
		}

		buffer <<= 5;    
		buffer |= ch;
		left += 5;
		
		if (left >= 8) {
			out[outlen] = (buffer >> (left - 8)) & 0xff;
			left -= 8;
			outlen++;
		}
	}
	out[outlen] = '\0';

	return outlen;
}
