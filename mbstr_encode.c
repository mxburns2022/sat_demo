#define _POSIX_C_SOURCE 200809
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
int nondet_int();
/*
   Multibyte encodings.

   In ASCII, a standard for encoding mostly English text data, each
   character (e.g. A to Z) occupies one byte. However, using one byte
   per character does not work for languages which may have a large
   number of characters.

   Such languages use a multibyte encoding where one character can
   occupy more than one byte. Many such multibyte encodings exist. One
   such encoding for a hypothetical language is described below:

   Assume that the characters of this language can fit in 21 bits (so
   characters range from 0 to 1114111).

   Then,

   For values from 0 to 127 (which take 7 bits), the encoding is:

   0xxx xxxx - where the xxx xxxx bit indicate the character.


   For values from 128 to 2047 (which take 11 bits), use two bytes:

   110x xxxx  10xx xxxx

   e.g. 128 (000 1000 0000) => 1100 0001 1000 0000
                                       ^   ^^ ^^^^

   For values from 2048 to 65535, use three bytes

   1110 xxxx  10xx xxxx  10xx xxxx


   For values from 65536 to 1114111, use four bytes

   1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx

   This is a variable-length multibyte encoding, where each character
   can occupy 1 to 4 bytes.  In such encodings, a one-to-one relation
   between a character and a byte does not exist. Functions that
   operate on multibyte strings have to be rewritten.
*/

/*
  Encode characters from 0 to 1114111 (decimal) in mbstr. Assume mbstr
  is an array at least 4 bytes long.

  Return the number of bytes in the encoded value (1 to 4).

  Return 0 if c is outside the valid range of characters.

  The encode routine must use the fewest number of bytes to encode a character.
 */

int encode(uint32_t c, uint8_t mbstr[]) {
  int length = 0;

  if(c < 128) {
	mbstr[0] = c;
	length = 1;
  }else if(c <= 2047) {
	mbstr[0] = 0xc0 | (c >> 6);
	mbstr[1] = 0x80 | (c & 0x3F);
	length = 2;
  }else if(c <= 65535) {
	mbstr[0] = 0xe0 | (c >> 12);
	mbstr[1] = 0x80 | ((c >> 6) & 0x3F);
	mbstr[2] = 0x80 | (c & 0x3F);

	length = 3;
  }
  else if(c <= 1114111) {
    //int test = 0b00111111;
    //int test2 = 0x3F;
	mbstr[0] = 0xf0 | (c >> 18);
	mbstr[1] = 0x80 | ((c >> 12) & 0x3F);
	mbstr[2] = 0x80 | ((c >> 6) & 0x3F);
	mbstr[3] = 0x80 | (c & 0x3F);

	length = 4;
  return length;
  }

  return length;
}
void check_encode() {
  //Testing values, each byte boundary is tested on both sides along with an intermediate value. UINT32_MAX is used to test for any overflow issues
  uint32_t testers[] = {0u,5u, 127u, 128u, 512u, 2047u, 2048u, 4098u, 65665u,65536u, 1010293u, 1114111u, 1114112u, UINT32_MAX};
  int numtests = sizeof(testers)/sizeof(uint32_t); //the number of tests to be run, fixed size
  //CBMC will unroll loop to fixed test count
  for (int i = 0; i < numtests; i++) {
    uint8_t bytes[4] = {0,0,0,0}; //data buffer
    uint32_t tester =  testers[i]; //test value for this run
    int len = encode(tester, bytes); //encode the function
    //Now we want to reassemble from the byte buffer to ensure the encoding is correct


    //If only one byte, then simply equal to the first array index
    int32_t _1target = bytes[0];
    // Want to check that unused bytes are 0
    int32_t otherbytes1 = bytes[1] | (bytes[2] | bytes[3]);
    //Check that the target matches if within correct bounds
    assert(tester > 127 || ((len == 1 && _1target == tester) && otherbytes1 == 0));


    //If 2 bytes used:
    // Mask each byte of the buffer and reassemble
    int32_t _2target_1 = (0b00011111 & bytes[0]);
    int32_t _2target_2 = (0b00111111 & bytes[1]);
    // Shift the first byte by 6 and perform bitwise or to reassemble
    int32_t _2target = (_2target_1 << 6) | _2target_2;
    // Want to check that unused bytes are 0
    int32_t otherbytes2 = bytes[2] | bytes[3];
    //Check that the target matches if within correct bounds
    assert((tester > 2047 || tester <= 127) || \
        ((len == 2 && _2target == tester) && otherbytes2 == 0));


    //If three bytes used:
      // Mask each byte according to encoding scheme
      int32_t _3target_1 = (0b00001111 & bytes[0]);
      int32_t _3target_2 = (0b00111111 & bytes[1]);
      int32_t _3target_3 = (0b00111111 & bytes[2]);
      //Unused byte
      int32_t otherbytes3 = bytes[3];
      // Shift the first masked byte by 12, the second masked byte by 6, and perform bitwise or to join all 3 bytes (done in 2 steps)
      int32_t _3target = (_3target_1 << 6) | _3target_2;
      _3target = (_3target << 6) | _3target_3;
    //Check that the target matches if within correct bounds
      assert((tester > 65535 || tester <= 2047) || \
          ((len == 3 && _3target == tester) && otherbytes3 == 0));


    //If 4 bytes were used:
    //Mask each byte in accordance w/ the encoding scheme
    int32_t _4target_1 = (0b00000111 & bytes[0]);
    int32_t _4target_2 = (0b00111111 & bytes[1]);
    int32_t _4target_3 = (0b00111111 & bytes[2]);
    int32_t _4target_4 = (0b00111111 & bytes[3]);
    //reassemble the encoded value
    int32_t _4target = (_4target_1 << 6) | _4target_2;
    _4target = (_4target << 6) | _4target_3;
    _4target = (_4target << 6) | _4target_4;
    //Check that the target matches if within correct bounds
    assert((tester > 1114111 || tester <= 65535) || \
        (len == 4 && _4target == tester));


    //If not in bounds, verify that the returned length was 0 and the buffer was not written to
    int32_t unused_too_large = bytes[0] | bytes[1] | bytes[2] | bytes[3];
    assert((tester <= 1114111) || (len == 0 && unused_too_large == 0));
  }

}
int main() {
  check_encode();
  return 0;
}