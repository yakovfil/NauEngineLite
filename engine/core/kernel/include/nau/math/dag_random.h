// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once
#include "nau/kernel/kernel_config.h"

#include <nau/kernel/kernel_config.h>

namespace dagor_random
{

#define DAGOR_RAND_MAX 32767

NAU_FORCE_INLINE int _rnd(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  return int((a >> 16) & DAGOR_RAND_MAX);
}

NAU_FORCE_INLINE float _frnd(int &s) { return float(_rnd(s)) / float(DAGOR_RAND_MAX + 1); }

NAU_FORCE_INLINE float _srnd(int &s) { return float(_rnd(s) * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1); }

NAU_FORCE_INLINE void _rnd_ivec(int &seed, int &x, int &y, int &z)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  z = int((a >> 16) & DAGOR_RAND_MAX);
  y = int((b >> 16) & DAGOR_RAND_MAX);
  x = int((c >> 16) & DAGOR_RAND_MAX);
  seed = (int)c;
}

NAU_FORCE_INLINE void _rnd_fvec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  x = float(ix) / float(DAGOR_RAND_MAX + 1);
  y = float(iy) / float(DAGOR_RAND_MAX + 1);
  z = float(iz) / float(DAGOR_RAND_MAX + 1);
}

NAU_FORCE_INLINE void _rnd_svec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  x = float(ix * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  y = float(iy * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  z = float(iz * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
}

NAU_FORCE_INLINE void _skip_rnd_ivec4(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  c = (unsigned)b * 0x41C64E6D + 0x3039;
  d = (unsigned)c * 0x41C64E6D + 0x3039;
  seed = (int)d;
}

NAU_FORCE_INLINE void _rnd_ivec4(int &seed, int &x, int &y, int &z, int &w)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  d = c * 0x41C64E6D + 0x3039;
  w = int((a >> 16) & DAGOR_RAND_MAX);
  z = int((b >> 16) & DAGOR_RAND_MAX);
  y = int((c >> 16) & DAGOR_RAND_MAX);
  x = int((d >> 16) & DAGOR_RAND_MAX);
  seed = (int)d;
}

NAU_FORCE_INLINE void _rnd_fvec4(int &seed, float &x, float &y, float &z, float &w)
{
  int ix, iy, iz, iw;
  _rnd_ivec4(seed, ix, iy, iz, iw);
  x = float(ix) / float(DAGOR_RAND_MAX + 1);
  y = float(iy) / float(DAGOR_RAND_MAX + 1);
  z = float(iz) / float(DAGOR_RAND_MAX + 1);
  w = float(iw) / float(DAGOR_RAND_MAX + 1);
}

NAU_FORCE_INLINE void _rnd_svec4(int &seed, float &x, float &y, float &z, float &w)
{
  int ix, iy, iz, iw;
  _rnd_ivec4(seed, ix, iy, iz, iw);
  x = float(ix * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  y = float(iy * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  z = float(iz * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  w = float(iw * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
}

NAU_FORCE_INLINE float _rnd_float(int &seed, float a, float b) { return a + (b - a) * _frnd(seed); }
NAU_FORCE_INLINE int _rnd_int(int &seed, int a, int b) { return a + (b - a + 1) * _rnd(seed) / (DAGOR_RAND_MAX + 1); }

//
// Gaussian random number
//
struct LineIntTbl
{
  float k_div_128, b;
};
extern NAU_KERNEL_EXPORT LineIntTbl g_gauss_table[3][256];

NAU_FORCE_INLINE float _gauss_rnd(int &seed, int n = 0)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  unsigned x = (a >> 16) & DAGOR_RAND_MAX, t = x >> 7;
  return g_gauss_table[n][t].b + g_gauss_table[n][t].k_div_128 * float(x & 0x7F);
}

NAU_FORCE_INLINE float _gauss_rnd_fast(int &seed, int n = 0)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  unsigned t = (a >> 23) & 0xFF;
  return g_gauss_table[n][t].b;
}

extern NAU_KERNEL_EXPORT int g_rnd_seed;


NAU_FORCE_INLINE void set_rnd_seed(int rnd_seed) { g_rnd_seed = rnd_seed; }
NAU_FORCE_INLINE int get_rnd_seed() { return g_rnd_seed; }

NAU_FORCE_INLINE int grnd() { return _rnd(g_rnd_seed); }
NAU_FORCE_INLINE float gfrnd() { return _frnd(g_rnd_seed); }
NAU_FORCE_INLINE float gsrnd() { return _srnd(g_rnd_seed); }

NAU_FORCE_INLINE float rnd_float(float a, float b) { return _rnd_float(g_rnd_seed, a, b); }
NAU_FORCE_INLINE int rnd_int(int a, int b) { return _rnd_int(g_rnd_seed, a, b); }
NAU_FORCE_INLINE void rnd_svec(float &x, float &y, float &z) { _rnd_svec(g_rnd_seed, x, y, z); }
NAU_FORCE_INLINE float gauss_rnd(int n = 0) { return _gauss_rnd(g_rnd_seed, n); }

} // namespace dagor_random
using namespace dagor_random;
