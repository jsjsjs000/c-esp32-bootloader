#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "crc32.h"

// const uint32_t Crc32Seed = 0x7e5291fb;
const uint32_t Crc32Table[] = {
	0x00000000, 0x580fbb03, 0x4cba55f1, 0x14b5eef2, 0x65d18815, 0x3dde3316, 0x296bdde4, 0x716466e7,
	0x370633dd, 0x6f0988de, 0x7bbc662c, 0x23b3dd2f, 0x52d7bbc8, 0x0ad800cb, 0x1e6dee39, 0x4662553a,
	0x6e0c67ba, 0x3603dcb9, 0x22b6324b, 0x7ab98948, 0x0bddefaf, 0x53d254ac, 0x4767ba5e, 0x1f68015d,
	0x590a5467, 0x0105ef64, 0x15b00196, 0x4dbfba95, 0x3cdbdc72, 0x64d46771, 0x70618983, 0x286e3280,
	0x20bdec83, 0x78b25780, 0x6c07b972, 0x34080271, 0x456c6496, 0x1d63df95, 0x09d63167, 0x51d98a64,
	0x17bbdf5e, 0x4fb4645d, 0x5b018aaf, 0x030e31ac, 0x726a574b, 0x2a65ec48, 0x3ed002ba, 0x66dfb9b9,
	0x4eb18b39, 0x16be303a, 0x020bdec8, 0x5a0465cb, 0x2b60032c, 0x736fb82f, 0x67da56dd, 0x3fd5edde,
	0x79b7b8e4, 0x21b803e7, 0x350ded15, 0x6d025616, 0x1c6630f1, 0x44698bf2, 0x50dc6500, 0x08d3de03,
	0x417bd906, 0x19746205, 0x0dc18cf7, 0x55ce37f4, 0x24aa5113, 0x7ca5ea10, 0x681004e2, 0x301fbfe1,
	0x767deadb, 0x2e7251d8, 0x3ac7bf2a, 0x62c80429, 0x13ac62ce, 0x4ba3d9cd, 0x5f16373f, 0x07198c3c,
	0x2f77bebc, 0x777805bf, 0x63cdeb4d, 0x3bc2504e, 0x4aa636a9, 0x12a98daa, 0x061c6358, 0x5e13d85b,
	0x18718d61, 0x407e3662, 0x54cbd890, 0x0cc46393, 0x7da00574, 0x25afbe77, 0x311a5085, 0x6915eb86,
	0x61c63585, 0x39c98e86, 0x2d7c6074, 0x7573db77, 0x0417bd90, 0x5c180693, 0x48ade861, 0x10a25362,
	0x56c00658, 0x0ecfbd5b, 0x1a7a53a9, 0x4275e8aa, 0x33118e4d, 0x6b1e354e, 0x7fabdbbc, 0x27a460bf,
	0x0fca523f, 0x57c5e93c, 0x437007ce, 0x1b7fbccd, 0x6a1bda2a, 0x32146129, 0x26a18fdb, 0x7eae34d8,
	0x38cc61e2, 0x60c3dae1, 0x74763413, 0x2c798f10, 0x5d1de9f7, 0x051252f4, 0x11a7bc06, 0x49a80705,
	0x7e5291fb, 0x265d2af8, 0x32e8c40a, 0x6ae77f09, 0x1b8319ee, 0x438ca2ed, 0x57394c1f, 0x0f36f71c,
	0x4954a226, 0x115b1925, 0x05eef7d7, 0x5de14cd4, 0x2c852a33, 0x748a9130, 0x603f7fc2, 0x3830c4c1,
	0x105ef641, 0x48514d42, 0x5ce4a3b0, 0x04eb18b3, 0x758f7e54, 0x2d80c557, 0x39352ba5, 0x613a90a6,
	0x2758c59c, 0x7f577e9f, 0x6be2906d, 0x33ed2b6e, 0x42894d89, 0x1a86f68a, 0x0e331878, 0x563ca37b,
	0x5eef7d78, 0x06e0c67b, 0x12552889, 0x4a5a938a, 0x3b3ef56d, 0x63314e6e, 0x7784a09c, 0x2f8b1b9f,
	0x69e94ea5, 0x31e6f5a6, 0x25531b54, 0x7d5ca057, 0x0c38c6b0, 0x54377db3, 0x40829341, 0x188d2842,
	0x30e31ac2, 0x68eca1c1, 0x7c594f33, 0x2456f430, 0x553292d7, 0x0d3d29d4, 0x1988c726, 0x41877c25,
	0x07e5291f, 0x5fea921c, 0x4b5f7cee, 0x1350c7ed, 0x6234a10a, 0x3a3b1a09, 0x2e8ef4fb, 0x76814ff8,
	0x3f2948fd, 0x6726f3fe, 0x73931d0c, 0x2b9ca60f, 0x5af8c0e8, 0x02f77beb, 0x16429519, 0x4e4d2e1a,
	0x082f7b20, 0x5020c023, 0x44952ed1, 0x1c9a95d2, 0x6dfef335, 0x35f14836, 0x2144a6c4, 0x794b1dc7,
	0x51252f47, 0x092a9444, 0x1d9f7ab6, 0x4590c1b5, 0x34f4a752, 0x6cfb1c51, 0x784ef2a3, 0x204149a0,
	0x66231c9a, 0x3e2ca799, 0x2a99496b, 0x7296f268, 0x03f2948f, 0x5bfd2f8c, 0x4f48c17e, 0x17477a7d,
	0x1f94a47e, 0x479b1f7d, 0x532ef18f, 0x0b214a8c, 0x7a452c6b, 0x224a9768, 0x36ff799a, 0x6ef0c299,
	0x289297a3, 0x709d2ca0, 0x6428c252, 0x3c277951, 0x4d431fb6, 0x154ca4b5, 0x01f94a47, 0x59f6f144,
	0x7198c3c4, 0x299778c7, 0x3d229635, 0x652d2d36, 0x14494bd1, 0x4c46f0d2, 0x58f31e20, 0x00fca523,
	0x469ef019, 0x1e914b1a, 0x0a24a5e8, 0x522b1eeb, 0x234f780c, 0x7b40c30f, 0x6ff52dfd, 0x37fa96fe };

// void InitializeCrc32Table()
// {
// 	uint32_t crc, d;
// 	for (uint32_t i = 0; i < 256; i++)
// 	{
// 		crc = 0;
// 		d = i;
// 		for (int j = 0; j < 8; j++)
// 		{
// 			if (((crc ^ d) & 0x0001) != 0)
// 				crc = (crc >> 1) ^ Crc32Seed;
// 			else
// 				crc >>= 1;
// 			d >>= 1;
// 		}
// 		Crc32Table[i] = crc;
// 	}
// }

uint32_t CalculateCrc32(uint32_t crc, uint8_t* data, int16_t from, int16_t length)
{
	for (int32_t i = from; i < from + length; i++)
		crc = (crc >> 8) ^ Crc32Table[(crc ^ data[i]) & 0xff];
	return crc;
}

uint32_t CalculateCrc32_1Byte(uint32_t crc, uint8_t data)
{
	return (crc >> 8) ^ Crc32Table[(crc ^ data) & 0xff];
}

uint32_t CalculateCrc32_1UInt(uint32_t crc, uint32_t data)
{
	crc = CalculateCrc32_1Byte(crc, UINT32_0BYTE(data));
	crc = CalculateCrc32_1Byte(crc, UINT32_1BYTE(data));
	crc = CalculateCrc32_1Byte(crc, UINT32_2BYTE(data));
	crc = CalculateCrc32_1Byte(crc, UINT32_3BYTE(data));
	return crc;
}