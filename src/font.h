#define FONT_COUNT 101

#ifndef FONT_H
#define FONT_H

/* This is what we have to work with */
/* Everything above default ascii values goes in raw fomat (extended ascii) */
char charLookup[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\xe6\xf8\xe5\xc6\xd8\xc5";

/* Defines all characters in the lookup string */
/* First byte is width of font, rest is padded with 0-bytes */
unsigned char font_variable[101][8] = {
    {1,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {1,0x5f,0x00,0x00,0x00,0x00,0x00,0x00}, /* ! */
    {3,0x07,0x00,0x07,0x00,0x00,0x00,0x00}, /* " */
    {5,0x14,0x3e,0x14,0x3e,0x14,0x00,0x00}, /* # */
    {5,0x24,0x2a,0x7f,0x2a,0x12,0x00,0x00}, /* $ */
    {7,0x42,0x25,0x12,0x08,0x24,0x52,0x21}, /* % */
    {7,0x30,0x4a,0x45,0x4d,0x52,0x20,0x50}, /* & */
    {1,0x07,0x00,0x00,0x00,0x00,0x00,0x00}, /* ' */
    {2,0x3e,0x41,0x00,0x00,0x00,0x00,0x00}, /* ( */
    {2,0x41,0x3e,0x00,0x00,0x00,0x00,0x00}, /* ) */
    {3,0x05,0x02,0x05,0x00,0x00,0x00,0x00}, /* * */
    {5,0x08,0x08,0x3e,0x08,0x08,0x00,0x00}, /* + */
    {2,0x80,0x60,0x00,0x00,0x00,0x00,0x00}, /* , */
    {4,0x08,0x08,0x08,0x08,0x00,0x00,0x00}, /* - */
    {2,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, /* . */
    {3,0x60,0x1c,0x03,0x00,0x00,0x00,0x00}, /* / */
    {5,0x3e,0x41,0x49,0x41,0x3e,0x00,0x00}, /* 0 */
    {3,0x42,0x7f,0x40,0x00,0x00,0x00,0x00}, /* 1 */
    {5,0x42,0x61,0x51,0x49,0x46,0x00,0x00}, /* 2 */
    {5,0x22,0x41,0x49,0x49,0x36,0x00,0x00}, /* 3 */
    {5,0x0f,0x08,0x08,0x7f,0x08,0x00,0x00}, /* 4 */
    {5,0x4f,0x49,0x49,0x49,0x31,0x00,0x00}, /* 5 */
    {5,0x3c,0x4a,0x49,0x49,0x30,0x00,0x00}, /* 6 */
    {4,0x01,0x61,0x19,0x07,0x00,0x00,0x00}, /* 7 */
    {5,0x36,0x49,0x49,0x49,0x36,0x00,0x00}, /* 8 */
    {5,0x06,0x09,0x09,0x09,0x7e,0x00,0x00}, /* 9 */
    {2,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, /* : */
    {3,0x80,0xe6,0x66,0x00,0x00,0x00,0x00}, /* ; */
    {4,0x08,0x14,0x14,0x22,0x00,0x00,0x00}, /* < */
    {4,0x14,0x14,0x14,0x14,0x00,0x00,0x00}, /* = */
    {4,0x22,0x14,0x14,0x08,0x00,0x00,0x00}, /* > */
    {5,0x02,0x01,0x51,0x09,0x06,0x00,0x00}, /* ? */
    {7,0x3c,0x42,0x99,0xab,0xbd,0xa2,0x1c}, /* @ */
    {5,0x7e,0x09,0x09,0x09,0x7e,0x00,0x00}, /* A */
    {5,0x7f,0x49,0x49,0x49,0x36,0x00,0x00}, /* B */
    {5,0x3e,0x41,0x41,0x41,0x22,0x00,0x00}, /* C */
    {5,0x7f,0x41,0x41,0x22,0x1c,0x00,0x00}, /* D */
    {4,0x7f,0x49,0x49,0x41,0x00,0x00,0x00}, /* E */
    {4,0x7f,0x09,0x09,0x01,0x00,0x00,0x00}, /* F */
    {5,0x3e,0x41,0x41,0x49,0x3a,0x00,0x00}, /* G */
    {5,0x7f,0x08,0x08,0x08,0x7f,0x00,0x00}, /* H */
    {3,0x41,0x7f,0x41,0x00,0x00,0x00,0x00}, /* I */
    {5,0x20,0x40,0x40,0x40,0x3f,0x00,0x00}, /* J */
    {5,0x7f,0x08,0x14,0x22,0x41,0x00,0x00}, /* K */
    {5,0x7f,0x40,0x40,0x40,0x40,0x00,0x00}, /* L */
    {5,0x7f,0x02,0x04,0x02,0x7f,0x00,0x00}, /* M */
    {5,0x7f,0x02,0x04,0x08,0x7f,0x00,0x00}, /* N */
    {5,0x3e,0x41,0x41,0x41,0x3e,0x00,0x00}, /* O */
    {5,0x7f,0x09,0x09,0x09,0x06,0x00,0x00}, /* P */
    {6,0x3e,0x41,0x51,0x61,0x7e,0x80,0x00}, /* Q */
    {5,0x7f,0x09,0x19,0x29,0x46,0x00,0x00}, /* R */
    {5,0x26,0x49,0x49,0x49,0x32,0x00,0x00}, /* S */
    {5,0x01,0x01,0x7f,0x01,0x01,0x00,0x00}, /* T */
    {5,0x3f,0x40,0x40,0x40,0x3f,0x00,0x00}, /* U */
    {5,0x1f,0x20,0x40,0x20,0x1f,0x00,0x00}, /* V */
    {5,0x1f,0x60,0x18,0x60,0x1f,0x00,0x00}, /* W */
    {5,0x63,0x14,0x08,0x14,0x63,0x00,0x00}, /* X */
    {5,0x03,0x04,0x78,0x04,0x03,0x00,0x00}, /* Y */
    {5,0x61,0x51,0x49,0x45,0x43,0x00,0x00}, /* Z */
    {2,0x7f,0x41,0x00,0x00,0x00,0x00,0x00}, /* [ */
    {3,0x03,0x1c,0x60,0x00,0x00,0x00,0x00}, /* slash */
    {2,0x41,0x7f,0x00,0x00,0x00,0x00,0x00}, /* ] */
    {5,0x04,0x02,0x01,0x02,0x04,0x00,0x00}, /* ^ */
    {4,0x80,0x80,0x80,0x80,0x00,0x00,0x00}, /* _ */
    {2,0x01,0x02,0x00,0x00,0x00,0x00,0x00}, /* ` */
    {4,0x20,0x54,0x54,0x78,0x00,0x00,0x00}, /* a */
    {4,0x7f,0x44,0x44,0x38,0x00,0x00,0x00}, /* b */
    {4,0x38,0x44,0x44,0x44,0x00,0x00,0x00}, /* c */
    {4,0x38,0x44,0x44,0x7f,0x00,0x00,0x00}, /* d */
    {4,0x38,0x54,0x54,0x18,0x00,0x00,0x00}, /* e */
    {3,0x08,0x7e,0x09,0x00,0x00,0x00,0x00}, /* f */
    {4,0x58,0xa4,0xa4,0x7c,0x00,0x00,0x00}, /* g */
    {4,0x7f,0x04,0x04,0x78,0x00,0x00,0x00}, /* h */
    {1,0x7d,0x00,0x00,0x00,0x00,0x00,0x00}, /* i */
    {2,0x80,0x7d,0x00,0x00,0x00,0x00,0x00}, /* j */
    {4,0x7f,0x10,0x28,0x44,0x00,0x00,0x00}, /* k */
    {1,0x7f,0x00,0x00,0x00,0x00,0x00,0x00}, /* l */
    {5,0x7c,0x04,0x78,0x04,0x78,0x00,0x00}, /* m */
    {4,0x7c,0x04,0x04,0x78,0x00,0x00,0x00}, /* n */
    {4,0x38,0x44,0x44,0x38,0x00,0x00,0x00}, /* o */
    {4,0xfc,0x24,0x24,0x18,0x00,0x00,0x00}, /* p */
    {4,0x18,0x24,0x24,0xfc,0x00,0x00,0x00}, /* q */
    {4,0x7c,0x04,0x04,0x08,0x00,0x00,0x00}, /* r */
    {4,0x48,0x54,0x54,0x24,0x00,0x00,0x00}, /* s */
    {3,0x04,0x3f,0x44,0x00,0x00,0x00,0x00}, /* t */
    {4,0x3c,0x40,0x40,0x7c,0x00,0x00,0x00}, /* u */
    {5,0x1c,0x20,0x40,0x20,0x1c,0x00,0x00}, /* v */
    {5,0x3c,0x40,0x3c,0x40,0x3c,0x00,0x00}, /* w */
    {5,0x44,0x28,0x10,0x28,0x44,0x00,0x00}, /* x */
    {4,0x5c,0xa0,0xa0,0x7c,0x00,0x00,0x00}, /* y */
    {3,0x64,0x54,0x4c,0x00,0x00,0x00,0x00}, /* z */
    {4,0x18,0x66,0x81,0x81,0x00,0x00,0x00}, /* { */
    {1,0xff,0x00,0x00,0x00,0x00,0x00,0x00}, /* | */
    {4,0x81,0x81,0x66,0x18,0x00,0x00,0x00}, /* } */
    {6,0x08,0x04,0x04,0x08,0x08,0x04,0x00}, /* ~ */
    {7,0x20,0x54,0x54,0x38,0x54,0x54,0x58}, /* \xe6 */
    {6,0x40,0x38,0x64,0x54,0x38,0x04,0x00}, /* \xf8 */
    {4,0x20,0x54,0x55,0x78,0x00,0x00,0x00}, /* \xe5 */
    {6,0x7e,0x09,0x09,0x7f,0x49,0x41,0x00}, /* \xc6 */
    {7,0x40,0x3e,0x51,0x49,0x45,0x3e,0x01}, /* \xd8 */
    {5,0x78,0x17,0x15,0x17,0x78,0x00,0x00}, /* \xc5 */
};

#endif
