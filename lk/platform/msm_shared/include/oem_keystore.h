/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __OEM_KEYSTORE_H
#define __OEM_KEYSTORE_H
#ifndef HQ_BUILD_FLAG //miui used key
const unsigned char OEM_KEYSTORE[] = {
	  0x30, 0x82, 0x06, 0x71, 0x02, 0x01, 0x00, 0x30, 0x82, 0x01, 0x1f, 0x30,
	  0x82, 0x01, 0x1b, 0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
	  0x0d, 0x01, 0x01, 0x0b, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
	  0x00, 0xee, 0x69, 0x53, 0xa8, 0x10, 0xe2, 0x08, 0xa7, 0x14, 0x41, 0xe2,
	  0xc9, 0x63, 0xd3, 0x1b, 0x8a, 0x4f, 0xbb, 0x76, 0xca, 0xbb, 0xc3, 0xce,
	  0xf1, 0xd0, 0x84, 0xe0, 0x25, 0x33, 0xcb, 0xf3, 0xfa, 0xb9, 0xd1, 0xff,
	  0xfe, 0xa2, 0x98, 0x9f, 0x84, 0x4f, 0x4e, 0xff, 0x74, 0xcb, 0xc0, 0x13,
	  0xcf, 0x77, 0x14, 0xce, 0x4b, 0x9c, 0x54, 0x0b, 0x35, 0xc5, 0x09, 0xf8,
	  0x56, 0x08, 0x1a, 0x36, 0xd8, 0x77, 0x31, 0x42, 0x55, 0x26, 0x04, 0xad,
	  0xf5, 0x53, 0xab, 0xde, 0x9e, 0xdf, 0xbf, 0xec, 0xc5, 0x54, 0xfc, 0xe9,
	  0x9b, 0xb3, 0xdf, 0xdd, 0xaa, 0xf0, 0xe0, 0x64, 0x63, 0x85, 0x05, 0xce,
	  0x93, 0x5d, 0xc3, 0x0c, 0xd6, 0xab, 0xa2, 0x7e, 0xd2, 0xd0, 0x2c, 0x13,
	  0x2b, 0xbb, 0xef, 0xc1, 0x02, 0x21, 0x38, 0x8f, 0x71, 0xe4, 0xc9, 0x82,
	  0x89, 0x7b, 0x23, 0x1c, 0xe3, 0x22, 0x58, 0xc7, 0x71, 0xc2, 0xb7, 0x7d,
	  0xf3, 0xfd, 0x75, 0x07, 0xf5, 0x17, 0x38, 0x81, 0xe8, 0x10, 0x53, 0x39,
	  0x00, 0x4a, 0x8f, 0x6e, 0x89, 0xd8, 0xa0, 0xaa, 0xb5, 0x29, 0x97, 0x33,
	  0x72, 0xa4, 0xe0, 0x67, 0x95, 0xd3, 0x07, 0x42, 0x31, 0x95, 0x14, 0x72,
	  0xe5, 0x97, 0x1a, 0xbb, 0x74, 0x52, 0x39, 0xab, 0x0f, 0xec, 0x47, 0x6b,
	  0xc8, 0xd2, 0x8f, 0xdf, 0x98, 0x45, 0xbe, 0xc1, 0x59, 0xef, 0x91, 0x7d,
	  0x80, 0x97, 0xb6, 0xaa, 0xc0, 0x6a, 0xb7, 0x9e, 0xfd, 0x0d, 0xb8, 0x18,
	  0x09, 0xb8, 0x9d, 0x2c, 0xe7, 0x0e, 0x37, 0xd2, 0x1f, 0x84, 0xf9, 0xd0,
	  0x98, 0x8d, 0x06, 0x46, 0x9e, 0x39, 0xff, 0x58, 0x67, 0x04, 0x25, 0x87,
	  0xae, 0xc9, 0xb4, 0x56, 0x5b, 0xb9, 0xe5, 0x0f, 0xe6, 0x9a, 0xe5, 0x43,
	  0x77, 0x97, 0x9a, 0xe6, 0xdd, 0x76, 0x87, 0x67, 0x03, 0x42, 0xdd, 0x0e,
	  0x43, 0x89, 0xa2, 0x80, 0x47, 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x82,
	  0x05, 0x47, 0x02, 0x01, 0x01, 0x30, 0x82, 0x04, 0x1f, 0x30, 0x82, 0x03,
	  0x07, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00, 0x80, 0x04, 0xec,
	  0xf9, 0x35, 0x98, 0x6a, 0x34, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
	  0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x81, 0xa5, 0x31,
	  0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53,
	  0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x0a, 0x43,
	  0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61, 0x31, 0x14, 0x30,
	  0x12, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0c, 0x0b, 0x53, 0x61, 0x6e, 0x20,
	  0x4e, 0x61, 0x72, 0x63, 0x69, 0x73, 0x6f, 0x31, 0x17, 0x30, 0x15, 0x06,
	  0x03, 0x55, 0x04, 0x0a, 0x0c, 0x0e, 0x59, 0x6f, 0x79, 0x6f, 0x64, 0x79,
	  0x6e, 0x65, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x31, 0x1a, 0x30, 0x18,
	  0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c, 0x11, 0x59, 0x6f, 0x79, 0x6f, 0x64,
	  0x79, 0x6e, 0x65, 0x20, 0x4d, 0x6f, 0x62, 0x69, 0x6c, 0x69, 0x74, 0x79,
	  0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x08, 0x59,
	  0x6f, 0x79, 0x6f, 0x64, 0x79, 0x6e, 0x65, 0x31, 0x23, 0x30, 0x21, 0x06,
	  0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x14,
	  0x79, 0x6f, 0x79, 0x6f, 0x64, 0x79, 0x6e, 0x65, 0x40, 0x65, 0x78, 0x61,
	  0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d,
	  0x31, 0x35, 0x30, 0x39, 0x32, 0x32, 0x30, 0x39, 0x32, 0x39, 0x32, 0x31,
	  0x5a, 0x17, 0x0d, 0x34, 0x33, 0x30, 0x32, 0x30, 0x37, 0x30, 0x39, 0x32,
	  0x39, 0x32, 0x31, 0x5a, 0x30, 0x81, 0xa5, 0x31, 0x0b, 0x30, 0x09, 0x06,
	  0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11,
	  0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x0a, 0x43, 0x61, 0x6c, 0x69, 0x66,
	  0x6f, 0x72, 0x6e, 0x69, 0x61, 0x31, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
	  0x04, 0x07, 0x0c, 0x0b, 0x53, 0x61, 0x6e, 0x20, 0x4e, 0x61, 0x72, 0x63,
	  0x69, 0x73, 0x6f, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x0a,
	  0x0c, 0x0e, 0x59, 0x6f, 0x79, 0x6f, 0x64, 0x79, 0x6e, 0x65, 0x2c, 0x20,
	  0x49, 0x6e, 0x63, 0x2e, 0x31, 0x1a, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04,
	  0x0b, 0x0c, 0x11, 0x59, 0x6f, 0x79, 0x6f, 0x64, 0x79, 0x6e, 0x65, 0x20,
	  0x4d, 0x6f, 0x62, 0x69, 0x6c, 0x69, 0x74, 0x79, 0x31, 0x11, 0x30, 0x0f,
	  0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x08, 0x59, 0x6f, 0x79, 0x6f, 0x64,
	  0x79, 0x6e, 0x65, 0x31, 0x23, 0x30, 0x21, 0x06, 0x09, 0x2a, 0x86, 0x48,
	  0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x14, 0x79, 0x6f, 0x79, 0x6f,
	  0x64, 0x79, 0x6e, 0x65, 0x40, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65,
	  0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
	  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03,
	  0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
	  0x00, 0xee, 0x69, 0x53, 0xa8, 0x10, 0xe2, 0x08, 0xa7, 0x14, 0x41, 0xe2,
	  0xc9, 0x63, 0xd3, 0x1b, 0x8a, 0x4f, 0xbb, 0x76, 0xca, 0xbb, 0xc3, 0xce,
	  0xf1, 0xd0, 0x84, 0xe0, 0x25, 0x33, 0xcb, 0xf3, 0xfa, 0xb9, 0xd1, 0xff,
	  0xfe, 0xa2, 0x98, 0x9f, 0x84, 0x4f, 0x4e, 0xff, 0x74, 0xcb, 0xc0, 0x13,
	  0xcf, 0x77, 0x14, 0xce, 0x4b, 0x9c, 0x54, 0x0b, 0x35, 0xc5, 0x09, 0xf8,
	  0x56, 0x08, 0x1a, 0x36, 0xd8, 0x77, 0x31, 0x42, 0x55, 0x26, 0x04, 0xad,
	  0xf5, 0x53, 0xab, 0xde, 0x9e, 0xdf, 0xbf, 0xec, 0xc5, 0x54, 0xfc, 0xe9,
	  0x9b, 0xb3, 0xdf, 0xdd, 0xaa, 0xf0, 0xe0, 0x64, 0x63, 0x85, 0x05, 0xce,
	  0x93, 0x5d, 0xc3, 0x0c, 0xd6, 0xab, 0xa2, 0x7e, 0xd2, 0xd0, 0x2c, 0x13,
	  0x2b, 0xbb, 0xef, 0xc1, 0x02, 0x21, 0x38, 0x8f, 0x71, 0xe4, 0xc9, 0x82,
	  0x89, 0x7b, 0x23, 0x1c, 0xe3, 0x22, 0x58, 0xc7, 0x71, 0xc2, 0xb7, 0x7d,
	  0xf3, 0xfd, 0x75, 0x07, 0xf5, 0x17, 0x38, 0x81, 0xe8, 0x10, 0x53, 0x39,
	  0x00, 0x4a, 0x8f, 0x6e, 0x89, 0xd8, 0xa0, 0xaa, 0xb5, 0x29, 0x97, 0x33,
	  0x72, 0xa4, 0xe0, 0x67, 0x95, 0xd3, 0x07, 0x42, 0x31, 0x95, 0x14, 0x72,
	  0xe5, 0x97, 0x1a, 0xbb, 0x74, 0x52, 0x39, 0xab, 0x0f, 0xec, 0x47, 0x6b,
	  0xc8, 0xd2, 0x8f, 0xdf, 0x98, 0x45, 0xbe, 0xc1, 0x59, 0xef, 0x91, 0x7d,
	  0x80, 0x97, 0xb6, 0xaa, 0xc0, 0x6a, 0xb7, 0x9e, 0xfd, 0x0d, 0xb8, 0x18,
	  0x09, 0xb8, 0x9d, 0x2c, 0xe7, 0x0e, 0x37, 0xd2, 0x1f, 0x84, 0xf9, 0xd0,
	  0x98, 0x8d, 0x06, 0x46, 0x9e, 0x39, 0xff, 0x58, 0x67, 0x04, 0x25, 0x87,
	  0xae, 0xc9, 0xb4, 0x56, 0x5b, 0xb9, 0xe5, 0x0f, 0xe6, 0x9a, 0xe5, 0x43,
	  0x77, 0x97, 0x9a, 0xe6, 0xdd, 0x76, 0x87, 0x67, 0x03, 0x42, 0xdd, 0x0e,
	  0x43, 0x89, 0xa2, 0x80, 0x47, 0x02, 0x03, 0x01, 0x00, 0x01, 0xa3, 0x50,
	  0x30, 0x4e, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04,
	  0x14, 0x55, 0x60, 0xe7, 0x86, 0x3b, 0x4d, 0x81, 0x18, 0xc2, 0xf1, 0xb0,
	  0x65, 0x59, 0x5c, 0xf9, 0x3b, 0xb2, 0x44, 0x79, 0x92, 0x30, 0x1f, 0x06,
	  0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x55, 0x60,
	  0xe7, 0x86, 0x3b, 0x4d, 0x81, 0x18, 0xc2, 0xf1, 0xb0, 0x65, 0x59, 0x5c,
	  0xf9, 0x3b, 0xb2, 0x44, 0x79, 0x92, 0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d,
	  0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0d, 0x06, 0x09,
	  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x03,
	  0x82, 0x01, 0x01, 0x00, 0x56, 0x11, 0x62, 0x88, 0x84, 0xb4, 0x66, 0x8f,
	  0x1a, 0x3b, 0x29, 0xe9, 0x01, 0xe7, 0x3d, 0x84, 0x95, 0x1f, 0x97, 0x98,
	  0x46, 0x2c, 0x58, 0x0c, 0x45, 0x2d, 0x1f, 0x5e, 0x01, 0x63, 0x5f, 0xac,
	  0xaa, 0x70, 0x3c, 0x4a, 0x7b, 0x1b, 0x0e, 0xed, 0x34, 0xe1, 0xb9, 0x22,
	  0x0e, 0xe4, 0x9c, 0x11, 0x8f, 0xd1, 0x7d, 0x28, 0x78, 0x6b, 0xa4, 0xcd,
	  0xa0, 0xae, 0xf2, 0x56, 0xf0, 0x86, 0xd6, 0xdf, 0xb8, 0x9a, 0x22, 0x12,
	  0x78, 0x89, 0x08, 0x58, 0xfc, 0xd3, 0xbe, 0x66, 0xa7, 0x76, 0x03, 0x9e,
	  0x10, 0x19, 0xa7, 0xa2, 0x15, 0xdf, 0xcb, 0x5c, 0x77, 0x44, 0xab, 0x66,
	  0xae, 0x98, 0x19, 0xf4, 0x8a, 0x60, 0x28, 0x1e, 0x83, 0x34, 0x2b, 0xf7,
	  0x51, 0xe8, 0x59, 0x2e, 0x3c, 0x4b, 0x3c, 0x1a, 0xd0, 0xb6, 0xa6, 0x7e,
	  0xe4, 0x1a, 0x56, 0x90, 0x91, 0x2a, 0x72, 0x5e, 0x53, 0xda, 0x1f, 0x60,
	  0xd6, 0x4e, 0x12, 0x3a, 0xb3, 0x71, 0xa3, 0x74, 0x50, 0x50, 0xae, 0xdd,
	  0xd7, 0x91, 0x0e, 0xe3, 0xed, 0x64, 0xc8, 0x60, 0x0d, 0x78, 0x10, 0xe3,
	  0x05, 0x72, 0xab, 0xde, 0xe3, 0x05, 0xa6, 0xa9, 0xca, 0x15, 0x24, 0x74,
	  0x5a, 0x7d, 0xd6, 0xed, 0xe9, 0xf3, 0xf5, 0xb3, 0x66, 0x7e, 0x3c, 0x9a,
	  0x96, 0xb9, 0x2a, 0xee, 0x0e, 0xdb, 0x5b, 0x41, 0x47, 0xfc, 0x98, 0x8b,
	  0xce, 0xe3, 0x29, 0x4f, 0xc2, 0x44, 0x44, 0x85, 0x6a, 0x6e, 0x98, 0x11,
	  0xb0, 0xba, 0x2f, 0xca, 0x90, 0x11, 0x0b, 0x2b, 0xed, 0x3b, 0x9d, 0x15,
	  0x49, 0x4e, 0x5a, 0xe9, 0x1a, 0x95, 0xde, 0xe7, 0xe7, 0x3d, 0x84, 0x2e,
	  0x07, 0xc6, 0xc9, 0x88, 0x86, 0x50, 0xd3, 0xd9, 0x1a, 0x92, 0x6d, 0x12,
	  0xbc, 0x81, 0x08, 0x46, 0x95, 0xa7, 0x07, 0xa2, 0x60, 0xbd, 0x62, 0x37,
	  0x07, 0xc1, 0x96, 0xee, 0x7d, 0xee, 0xad, 0xfb, 0x30, 0x0b, 0x06, 0x09,
	  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x30, 0x0e, 0x13,
	  0x08, 0x6b, 0x65, 0x79, 0x73, 0x74, 0x6f, 0x72, 0x65, 0x02, 0x02, 0x01,
	  0x2a, 0x04, 0x82, 0x01, 0x00, 0x43, 0x7f, 0x66, 0xa8, 0xdf, 0x82, 0x32,
	  0xa8, 0xfa, 0x42, 0xa4, 0xb2, 0xe3, 0x82, 0x5a, 0xcd, 0x47, 0x9d, 0xbc,
	  0x29, 0x7b, 0x5e, 0xe6, 0x70, 0x11, 0xcf, 0x01, 0xf6, 0x69, 0xa8, 0xf1,
	  0xae, 0x16, 0x95, 0x85, 0xe4, 0x90, 0x4b, 0x6c, 0x26, 0x3f, 0x7d, 0x1f,
	  0x53, 0x67, 0x0b, 0xf6, 0xe9, 0xcb, 0x2d, 0xd7, 0xd8, 0x5c, 0x07, 0x94,
	  0x19, 0xcf, 0x73, 0x74, 0xa3, 0x89, 0x39, 0xc8, 0x9c, 0x9e, 0xab, 0x29,
	  0xe4, 0x54, 0x83, 0x7f, 0x30, 0x5d, 0x93, 0xce, 0x8f, 0x14, 0x83, 0x46,
	  0xff, 0x92, 0xd2, 0x14, 0x97, 0x73, 0x78, 0xe2, 0xd3, 0x24, 0x5b, 0x50,
	  0x3e, 0x62, 0x5a, 0x09, 0xba, 0x44, 0x59, 0x96, 0x23, 0x26, 0x77, 0xe1,
	  0x49, 0x96, 0xe3, 0x57, 0xce, 0x1b, 0xd5, 0x74, 0xd3, 0x11, 0x2c, 0x88,
	  0xd9, 0x00, 0xb2, 0x25, 0x97, 0x65, 0xad, 0x3d, 0x7c, 0x88, 0x6d, 0xba,
	  0x46, 0xc5, 0x3e, 0x9f, 0xa3, 0x37, 0x4d, 0xf1, 0x3a, 0x60, 0xf6, 0x1c,
	  0x3d, 0xd6, 0xc7, 0x32, 0x91, 0x7f, 0xfa, 0xa6, 0x02, 0xa0, 0xba, 0x2a,
	  0xcb, 0xf2, 0xf1, 0xa0, 0x06, 0x20, 0x2e, 0x5e, 0xad, 0x19, 0x93, 0x08,
	  0x26, 0x0f, 0x27, 0x01, 0x3c, 0x0a, 0xa1, 0xde, 0xa5, 0x9b, 0x29, 0xe6,
	  0x6e, 0xe6, 0x2c, 0x8b, 0xf7, 0xf3, 0xfb, 0xf4, 0x13, 0x7c, 0x8a, 0x09,
	  0x1e, 0xab, 0x7c, 0x18, 0xed, 0xbf, 0x33, 0x13, 0x5d, 0xfc, 0x0c, 0x77,
	  0x66, 0x0c, 0xb5, 0xdc, 0x39, 0x46, 0xe6, 0xe2, 0x19, 0xca, 0xcb, 0x5e,
	  0x85, 0x66, 0xdb, 0xe4, 0x05, 0xf6, 0x70, 0x2e, 0xdf, 0x6a, 0x3a, 0x37,
	  0x84, 0xa0, 0x55, 0x33, 0x33, 0xe6, 0x8b, 0xea, 0x60, 0x40, 0xf3, 0x03,
	  0x1f, 0xf6, 0xb5, 0x4e, 0xed, 0x37, 0xff, 0xdf, 0xe7, 0x3a, 0x1c, 0x3e,
	  0x46, 0xb0, 0xaa, 0xe5, 0x75, 0x03, 0x98, 0xd7, 0x50

};
#else //HQ used key
const unsigned char OEM_KEYSTORE[] = {
  0x30, 0x82, 0x06, 0x47, 0x02, 0x01, 0x00, 0x30, 0x82, 0x01, 0x1f, 0x30,
  0x82, 0x01, 0x1b, 0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x01, 0x01, 0x0b, 0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
  0x00, 0xa9, 0x82, 0x5a, 0x9d, 0xb2, 0x32, 0xee, 0x89, 0x66, 0x7d, 0xf7,
  0x8c, 0x5f, 0x20, 0x39, 0x3f, 0x7e, 0x81, 0x59, 0xcf, 0x3f, 0xd5, 0xcf,
  0x19, 0xfe, 0x4a, 0xb4, 0xd7, 0x9b, 0x43, 0x7b, 0x12, 0x89, 0xfc, 0xe3,
  0x3e, 0x3b, 0xe9, 0xc9, 0xa0, 0xbe, 0xd2, 0x6e, 0x29, 0xa6, 0x32, 0x4f,
  0xf1, 0x13, 0xa1, 0x00, 0x5a, 0x75, 0x23, 0x63, 0x90, 0xd7, 0x73, 0x43,
  0x0c, 0xda, 0x1b, 0xf2, 0xb9, 0x81, 0x51, 0x53, 0x29, 0xdb, 0x66, 0x45,
  0x08, 0x3b, 0x81, 0x55, 0x1a, 0xb7, 0x72, 0xfa, 0x81, 0xbe, 0xb9, 0xd6,
  0x23, 0x27, 0xae, 0x17, 0xba, 0xe0, 0x72, 0xe2, 0x3c, 0x11, 0x80, 0x8f,
  0xd7, 0x01, 0xb4, 0xe3, 0xe2, 0x65, 0x54, 0x0e, 0x07, 0x4a, 0x9f, 0x65,
  0xd5, 0x60, 0x8b, 0xdc, 0x61, 0xe5, 0x4d, 0xf9, 0x0c, 0x7a, 0xf7, 0x6c,
  0x91, 0xa0, 0xae, 0x5a, 0x89, 0x6d, 0x04, 0x04, 0x9c, 0x42, 0xfa, 0xee,
  0xde, 0xce, 0x19, 0xf3, 0x3b, 0xa0, 0x42, 0x61, 0xdf, 0x9e, 0x53, 0x21,
  0x0b, 0x22, 0xa9, 0x01, 0x20, 0x93, 0xd8, 0xe8, 0xfa, 0xd3, 0x22, 0x61,
  0x11, 0x1f, 0xe7, 0xed, 0x3c, 0x68, 0x3f, 0xa7, 0xf8, 0xc2, 0x61, 0x87,
  0x58, 0xf2, 0x2a, 0xf4, 0xd7, 0x71, 0x66, 0x18, 0x4d, 0xba, 0xa6, 0x83,
  0x47, 0xfe, 0xc4, 0xf1, 0xf7, 0x92, 0x90, 0xf8, 0x0f, 0x35, 0xae, 0xcd,
  0x05, 0x46, 0xaf, 0xd6, 0x1b, 0x87, 0x02, 0xfa, 0x16, 0x8b, 0x3a, 0x49,
  0xbb, 0x7e, 0x05, 0xbc, 0x48, 0xe6, 0x15, 0xbb, 0x2e, 0xc4, 0x67, 0x23,
  0xb8, 0x9a, 0xa5, 0xdc, 0x18, 0x9c, 0x8f, 0x1e, 0x1c, 0xf8, 0x6d, 0x43,
  0x54, 0x1e, 0xc8, 0x3a, 0xe2, 0x15, 0x5b, 0x53, 0x7a, 0xd1, 0xe8, 0xfb,
  0x13, 0x84, 0x20, 0x8d, 0xf0, 0xe2, 0xc6, 0xd4, 0x2f, 0x5e, 0x16, 0x17,
  0x68, 0x61, 0xac, 0xaa, 0x9d, 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x82,
  0x05, 0x1d, 0x02, 0x01, 0x01, 0x30, 0x82, 0x03, 0xf5, 0x30, 0x82, 0x02,
  0xdd, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00, 0xfc, 0x64, 0x50,
  0x06, 0x1d, 0x77, 0x43, 0x50, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
  0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x81, 0x90, 0x31,
  0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43, 0x4e,
  0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x08, 0x53,
  0x68, 0x61, 0x6e, 0x67, 0x48, 0x61, 0x69, 0x31, 0x11, 0x30, 0x0f, 0x06,
  0x03, 0x55, 0x04, 0x07, 0x0c, 0x08, 0x53, 0x68, 0x61, 0x6e, 0x67, 0x48,
  0x61, 0x69, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c,
  0x06, 0x41, 0x4c, 0x31, 0x35, 0x31, 0x32, 0x31, 0x0f, 0x30, 0x0d, 0x06,
  0x03, 0x55, 0x04, 0x0b, 0x0c, 0x06, 0x58, 0x69, 0x61, 0x6f, 0x4d, 0x69,
  0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x06, 0x48,
  0x75, 0x61, 0x51, 0x69, 0x6e, 0x31, 0x28, 0x30, 0x26, 0x06, 0x09, 0x2a,
  0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x19, 0x7a, 0x68,
  0x6f, 0x6e, 0x67, 0x78, 0x69, 0x61, 0x6e, 0x67, 0x63, 0x68, 0x61, 0x6f,
  0x40, 0x68, 0x75, 0x61, 0x71, 0x69, 0x6e, 0x2e, 0x63, 0x6f, 0x6d, 0x30,
  0x1e, 0x17, 0x0d, 0x31, 0x37, 0x30, 0x36, 0x32, 0x31, 0x30, 0x36, 0x34,
  0x31, 0x32, 0x35, 0x5a, 0x17, 0x0d, 0x34, 0x34, 0x31, 0x31, 0x30, 0x36,
  0x30, 0x36, 0x34, 0x31, 0x32, 0x35, 0x5a, 0x30, 0x81, 0x90, 0x31, 0x0b,
  0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43, 0x4e, 0x31,
  0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x08, 0x53, 0x68,
  0x61, 0x6e, 0x67, 0x48, 0x61, 0x69, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03,
  0x55, 0x04, 0x07, 0x0c, 0x08, 0x53, 0x68, 0x61, 0x6e, 0x67, 0x48, 0x61,
  0x69, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x06,
  0x41, 0x4c, 0x31, 0x35, 0x31, 0x32, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03,
  0x55, 0x04, 0x0b, 0x0c, 0x06, 0x58, 0x69, 0x61, 0x6f, 0x4d, 0x69, 0x31,
  0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x06, 0x48, 0x75,
  0x61, 0x51, 0x69, 0x6e, 0x31, 0x28, 0x30, 0x26, 0x06, 0x09, 0x2a, 0x86,
  0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x19, 0x7a, 0x68, 0x6f,
  0x6e, 0x67, 0x78, 0x69, 0x61, 0x6e, 0x67, 0x63, 0x68, 0x61, 0x6f, 0x40,
  0x68, 0x75, 0x61, 0x71, 0x69, 0x6e, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x82,
  0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
  0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82,
  0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xa9, 0x82, 0x5a, 0x9d, 0xb2,
  0x32, 0xee, 0x89, 0x66, 0x7d, 0xf7, 0x8c, 0x5f, 0x20, 0x39, 0x3f, 0x7e,
  0x81, 0x59, 0xcf, 0x3f, 0xd5, 0xcf, 0x19, 0xfe, 0x4a, 0xb4, 0xd7, 0x9b,
  0x43, 0x7b, 0x12, 0x89, 0xfc, 0xe3, 0x3e, 0x3b, 0xe9, 0xc9, 0xa0, 0xbe,
  0xd2, 0x6e, 0x29, 0xa6, 0x32, 0x4f, 0xf1, 0x13, 0xa1, 0x00, 0x5a, 0x75,
  0x23, 0x63, 0x90, 0xd7, 0x73, 0x43, 0x0c, 0xda, 0x1b, 0xf2, 0xb9, 0x81,
  0x51, 0x53, 0x29, 0xdb, 0x66, 0x45, 0x08, 0x3b, 0x81, 0x55, 0x1a, 0xb7,
  0x72, 0xfa, 0x81, 0xbe, 0xb9, 0xd6, 0x23, 0x27, 0xae, 0x17, 0xba, 0xe0,
  0x72, 0xe2, 0x3c, 0x11, 0x80, 0x8f, 0xd7, 0x01, 0xb4, 0xe3, 0xe2, 0x65,
  0x54, 0x0e, 0x07, 0x4a, 0x9f, 0x65, 0xd5, 0x60, 0x8b, 0xdc, 0x61, 0xe5,
  0x4d, 0xf9, 0x0c, 0x7a, 0xf7, 0x6c, 0x91, 0xa0, 0xae, 0x5a, 0x89, 0x6d,
  0x04, 0x04, 0x9c, 0x42, 0xfa, 0xee, 0xde, 0xce, 0x19, 0xf3, 0x3b, 0xa0,
  0x42, 0x61, 0xdf, 0x9e, 0x53, 0x21, 0x0b, 0x22, 0xa9, 0x01, 0x20, 0x93,
  0xd8, 0xe8, 0xfa, 0xd3, 0x22, 0x61, 0x11, 0x1f, 0xe7, 0xed, 0x3c, 0x68,
  0x3f, 0xa7, 0xf8, 0xc2, 0x61, 0x87, 0x58, 0xf2, 0x2a, 0xf4, 0xd7, 0x71,
  0x66, 0x18, 0x4d, 0xba, 0xa6, 0x83, 0x47, 0xfe, 0xc4, 0xf1, 0xf7, 0x92,
  0x90, 0xf8, 0x0f, 0x35, 0xae, 0xcd, 0x05, 0x46, 0xaf, 0xd6, 0x1b, 0x87,
  0x02, 0xfa, 0x16, 0x8b, 0x3a, 0x49, 0xbb, 0x7e, 0x05, 0xbc, 0x48, 0xe6,
  0x15, 0xbb, 0x2e, 0xc4, 0x67, 0x23, 0xb8, 0x9a, 0xa5, 0xdc, 0x18, 0x9c,
  0x8f, 0x1e, 0x1c, 0xf8, 0x6d, 0x43, 0x54, 0x1e, 0xc8, 0x3a, 0xe2, 0x15,
  0x5b, 0x53, 0x7a, 0xd1, 0xe8, 0xfb, 0x13, 0x84, 0x20, 0x8d, 0xf0, 0xe2,
  0xc6, 0xd4, 0x2f, 0x5e, 0x16, 0x17, 0x68, 0x61, 0xac, 0xaa, 0x9d, 0x02,
  0x03, 0x01, 0x00, 0x01, 0xa3, 0x50, 0x30, 0x4e, 0x30, 0x1d, 0x06, 0x03,
  0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x4c, 0x6d, 0xce, 0xfc, 0x9e,
  0x59, 0x3e, 0x12, 0x41, 0xf8, 0x51, 0xf3, 0x28, 0xe4, 0x1c, 0xf8, 0x7f,
  0x3a, 0x56, 0x6a, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18,
  0x30, 0x16, 0x80, 0x14, 0x4c, 0x6d, 0xce, 0xfc, 0x9e, 0x59, 0x3e, 0x12,
  0x41, 0xf8, 0x51, 0xf3, 0x28, 0xe4, 0x1c, 0xf8, 0x7f, 0x3a, 0x56, 0x6a,
  0x30, 0x0c, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01,
  0x01, 0xff, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
  0x01, 0x01, 0x05, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x6f, 0xab,
  0xe8, 0x75, 0x5b, 0x40, 0x44, 0xb6, 0x88, 0x28, 0x86, 0x02, 0x49, 0x8d,
  0x0f, 0x81, 0x54, 0x39, 0x56, 0x54, 0x79, 0xa7, 0x38, 0x7e, 0x82, 0x39,
  0x6b, 0x2b, 0x9d, 0xa3, 0x10, 0xd5, 0x84, 0xe6, 0x84, 0x46, 0x90, 0x55,
  0xc3, 0xab, 0x8e, 0x3b, 0xa7, 0xa0, 0x9e, 0x1d, 0x4f, 0x68, 0xbe, 0x6b,
  0x97, 0x74, 0x35, 0xd7, 0xcb, 0xa6, 0x74, 0x10, 0x15, 0xcc, 0xbe, 0xa0,
  0x6e, 0x64, 0xda, 0x1a, 0x3b, 0xcb, 0xaa, 0xea, 0xd3, 0xed, 0xae, 0x4b,
  0x9f, 0xf5, 0x1d, 0x33, 0x24, 0x15, 0x7c, 0x0f, 0xbd, 0x65, 0xb8, 0x29,
  0x60, 0x72, 0x97, 0x48, 0xd9, 0xbb, 0x5f, 0x66, 0xae, 0x1b, 0xd2, 0xb9,
  0x7a, 0x0d, 0xf6, 0xec, 0x76, 0x15, 0x32, 0xa7, 0x18, 0x32, 0x95, 0xc1,
  0xc8, 0x7b, 0xc3, 0xdf, 0x65, 0x7b, 0x1e, 0x0c, 0xed, 0xd9, 0xec, 0x4d,
  0x96, 0x4e, 0xe7, 0x9d, 0xe2, 0xd5, 0xb5, 0xf3, 0xc1, 0x0b, 0x11, 0xa7,
  0x88, 0x30, 0x58, 0x92, 0xd6, 0xf3, 0x40, 0x3c, 0x97, 0x1d, 0x9d, 0x1c,
  0xf5, 0x4d, 0x3a, 0x34, 0x2c, 0x6b, 0xc4, 0xa5, 0x74, 0x1f, 0x1f, 0x74,
  0x99, 0xbe, 0xa7, 0x0e, 0x0d, 0xa4, 0x5a, 0x82, 0x0a, 0xe6, 0x8c, 0xba,
  0xa0, 0x07, 0xd4, 0xd6, 0x6c, 0x1e, 0xdc, 0xd9, 0xf3, 0x19, 0x28, 0xde,
  0x57, 0x1a, 0x51, 0x30, 0x00, 0xf0, 0xac, 0xc5, 0x81, 0xcc, 0xb8, 0x11,
  0x13, 0x1d, 0xfd, 0x1a, 0x2a, 0xf9, 0x5c, 0xc5, 0xe3, 0x8d, 0xbd, 0xec,
  0x8f, 0x39, 0x5e, 0xfc, 0x2e, 0x03, 0x05, 0x5c, 0x2b, 0xae, 0xba, 0x62,
  0xde, 0x98, 0x16, 0xad, 0xd9, 0xe7, 0x41, 0xdc, 0xa0, 0x04, 0x0b, 0xba,
  0xde, 0x7e, 0x26, 0x83, 0x74, 0x11, 0xd0, 0xdc, 0xa7, 0xc5, 0x48, 0x83,
  0x1e, 0x4f, 0x98, 0x98, 0x0b, 0x05, 0xb8, 0xce, 0x2a, 0xff, 0xea, 0x4b,
  0xac, 0xcb, 0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
  0x01, 0x01, 0x0b, 0x30, 0x0e, 0x13, 0x08, 0x6b, 0x65, 0x79, 0x73, 0x74,
  0x6f, 0x72, 0x65, 0x02, 0x02, 0x01, 0x2a, 0x04, 0x82, 0x01, 0x00, 0xa0,
  0xe8, 0xaa, 0xf8, 0x28, 0x56, 0xf5, 0x9e, 0x5f, 0x5e, 0x8b, 0x8b, 0x34,
  0x55, 0x8c, 0x09, 0x91, 0xd0, 0x20, 0x91, 0xe2, 0x97, 0x41, 0x39, 0xbb,
  0x89, 0x4b, 0x8c, 0x6a, 0x36, 0x74, 0xda, 0x45, 0x4f, 0x14, 0x82, 0x7e,
  0x2a, 0xf3, 0xa8, 0xf3, 0x84, 0x2a, 0x95, 0x1f, 0x3d, 0x4a, 0x2c, 0x43,
  0x64, 0xb8, 0xe4, 0x37, 0x26, 0xc5, 0x26, 0xd5, 0x19, 0xe1, 0x41, 0x71,
  0x00, 0x19, 0xcf, 0x43, 0x63, 0x33, 0x7a, 0x29, 0xd4, 0x76, 0xec, 0x5e,
  0xf7, 0xef, 0x9f, 0xe5, 0x4c, 0xbf, 0x75, 0xa4, 0xfa, 0xf8, 0x66, 0xae,
  0x29, 0x4b, 0x4b, 0x17, 0x48, 0x81, 0xb3, 0x93, 0x63, 0x29, 0xf7, 0x09,
  0xdf, 0x1b, 0x3d, 0xb1, 0x25, 0x33, 0x75, 0xe3, 0x4d, 0x4d, 0x80, 0xb8,
  0x45, 0x7b, 0xcb, 0xd4, 0x2e, 0xf4, 0xd5, 0x85, 0xa2, 0x51, 0x9e, 0x82,
  0xc2, 0xa0, 0xf3, 0x48, 0xc5, 0x6f, 0x0c, 0xf4, 0xd2, 0x20, 0x3f, 0xe9,
  0x02, 0x09, 0x29, 0x55, 0x73, 0x48, 0xdf, 0x2d, 0x43, 0x0b, 0x15, 0xbf,
  0x92, 0x28, 0xfe, 0x3a, 0xb5, 0x39, 0x78, 0x5c, 0xcf, 0xd7, 0x08, 0x78,
  0x36, 0xc7, 0xe5, 0x5e, 0xa6, 0x6b, 0x6a, 0xc8, 0xc6, 0xaf, 0x1b, 0xab,
  0x1d, 0x94, 0xe6, 0x2a, 0xcc, 0xc0, 0x29, 0x99, 0xa4, 0x82, 0xad, 0x51,
  0xee, 0xb2, 0x60, 0x96, 0x58, 0x54, 0x48, 0x86, 0xc4, 0x0a, 0x21, 0x9f,
  0xa9, 0xee, 0xe2, 0x67, 0xed, 0x33, 0x45, 0xf5, 0x21, 0x3a, 0xc8, 0x25,
  0x61, 0x8a, 0xce, 0x32, 0x29, 0x3b, 0x7e, 0x78, 0xc0, 0x6a, 0x0b, 0xf7,
  0x4c, 0x1b, 0x44, 0xc5, 0xf0, 0x96, 0xa4, 0xa3, 0xcd, 0xa9, 0x15, 0xe0,
  0xe7, 0x7b, 0xb2, 0x4e, 0x6c, 0xc8, 0x82, 0xee, 0x9f, 0xfa, 0x37, 0xda,
  0xf2, 0x94, 0x0a, 0xe5, 0xe9, 0x01, 0xc0, 0xb2, 0x5d, 0x5b, 0x1b, 0x93,
  0x3e, 0x16, 0x6b
};

#endif
#endif
