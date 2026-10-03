// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#if !(_TARGET_PC | _TARGET_XBOX)
  !error: must not be included for non-PC target!
#endif

#include <vector>

#include "nau/3d/dag_drv3d.h"
//#include <generic/dag_tabFwd.h>


  namespace d3d
  {
#if _TARGET_PC_WIN
  VPROG create_vertex_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile, nau::string *out_err = NULL);
  FSHADER create_pixel_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
    nau::string *out_err = NULL);
  bool compile_compute_shader_hlsl(const char* hlsl_text, unsigned len, const char* entry, const char* profile,
                                   std::vector<uint32_t>& shader_bin, nau::string& out_err);
#endif

#if !_TARGET_D3D_MULTI
  VDECL get_program_vdecl(PROGRAM);

  bool set_vertex_shader(VPROG ps);
  bool set_pixel_shader(FSHADER ps);
#if _TARGET_PC_WIN
  namespace pcwin32
  {
  //! return D3DFORMAT for given texture
  unsigned get_texture_format(BaseTexture *tex);
  //! return D3DFORMAT for given texture as string
  const char *get_texture_format_str(BaseTexture *tex);
  void *get_native_surface(BaseTexture *tex);
  } // namespace pcwin32
#endif

  VPROG create_vertex_shader_asm(const char *asm_text);
  VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n);

  FSHADER create_pixel_shader_asm(const char *asm_text);
  FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n);

#if _TARGET_PC_WIN
  // additional d3d::pcwin32 interface (PC specific)
  namespace pcwin32
  {
  void set_present_wnd(void *hwnd);

  // set capture whole framebuffer with capture_screen(), not only window data.
  bool set_capture_full_frame_buffer(bool ison); // returns previous state
  }                                              // namespace pcwin32
#endif
  //! returns current state of VSYNC
  bool get_vsync_enabled();
  //! enables or disables strong VSYNC (flips only on VBLANK); returns true on success
  bool enable_vsync(bool enable);
  //! retrieve list of available display modes
  void get_video_modes_list(std::vector<nau::string> & list);
#endif
  }; // namespace d3d
