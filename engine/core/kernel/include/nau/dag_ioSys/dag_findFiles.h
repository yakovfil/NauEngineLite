// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <nau/kernel/kernel_config.h>

#include <string>
#include <vector>
namespace nau::iosys
{
    //! fills 'out_list' with filepathes of files in 'dir_path' with matching 'file_ext_to_match';
    //! 'vromfs' controls search through loaded VROMFS data, 'realfs' controls search through ordinary filesystem;
    //! 'subdirs' controls hierarchical search;
    //! returns number of added filepathes;
    NAU_KERNEL_EXPORT int find_files_in_folder(std::vector<std::string>& out_list, const char* dir_path, const char* file_suffix_to_match = "", bool vromfs = true, bool realfs = true, bool subdirs = false);

    //! fills 'out_list' with filepathes of files with specific name in all mounted vromfs
    //! returns number of added filepathes;
    NAU_KERNEL_EXPORT int find_file_in_vromfs(std::vector<std::string>& out_list, const char* filename);
}  // namespace nau::iosys