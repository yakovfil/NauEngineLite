// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <nau/dag_ioSys/dag_findFiles.h>

#include <string>
#include <vector>
// #include <nau/osApiWrappers/dag_vromfs.h>
#include <nau/osApiWrappers/basePath.h>
#include <nau/osApiWrappers/dag_basePath.h>
#include <nau/osApiWrappers/dag_direct.h>
#include <nau/osApiWrappers/dag_localConv.h>

#include "file_list_utils.h"
#include "nau/diag/logging.h"
#include "nau/string/string.h"
namespace nau::iosys
{
    using namespace nau::hal;
    static void find_real_files_in_folder(std::vector<std::string>& out_list, const char* dir_path, const char* file_suffix_to_match, bool subdirs)
    {
        std::string tmpPath = std::string(dir_path) + "/*" + file_suffix_to_match;
        alefind_t ff;
        if (dd_find_first(tmpPath.c_str(), 0, &ff))
        {
            do
                out_list.push_back(std::string(*dir_path ? dir_path : ".") + "/" + ff.name);
            while (dd_find_next(&ff));
            dd_find_close(&ff);
        }

        if (subdirs)
        {
            tmpPath = std::string(dir_path) + "/*";
            if (dd_find_first(tmpPath.c_str(), DA_SUBDIR, &ff))
            {
                do
                    if (ff.attr & DA_SUBDIR)
                    {
                        if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0 || dd_stricmp(ff.name, ".git") == 0 ||
                            dd_stricmp(ff.name, ".") == 0 || dd_stricmp(ff.name, "..") == 0)
                            continue;

                        std::string dirPath = std::string(dir_path) + "/" + ff.name;
                        find_real_files_in_folder(out_list, dirPath.c_str(), file_suffix_to_match, true);
                    }
                while (dd_find_next(&ff));
                dd_find_close(&ff);
            }
        }
    }

    static void find_vromfs_files_in_folder(std::vector<std::string>& out_list, const char* dir_path, const char* file_suffix_to_match, bool subdirs)
    {
        // TODO: VROM integration
        /*
        for (int bpi = -1; bpi < DF_MAX_BASE_PATH_NUM; bpi++)
        {
          const char *base_path_prefix = "";
          if (bpi >= 0)
          {
            NAU_LOG_DEBUG(nau::string::format(nau::string(u8"df_base_path[{}]={} df_base_path_vrom_mounted={}"), bpi, nau::string(df_base_path[bpi]), df_base_path_vrom_mounted[bpi]));
            if (!df_base_path[bpi])
              break;
            if (!df_base_path_vrom_mounted[bpi] || !*df_base_path[bpi])
              continue;
            base_path_prefix = df_base_path[bpi];
          }

          int suffix_len = static_cast<int>(strlen(file_suffix_to_match));
          std::string prefix("%s%s/", base_path_prefix, *dir_path ? dir_path : ".");
          std::string suffix(file_suffix_to_match);
          dd_simplify_fname_c(prefix.data());
          prefix.resize(prefix.size());
          dd_strlwr(prefix.data());
          dd_strlwr(suffix.data());

          iterate_vroms([&](VirtualRomFsData *entry, size_t) {
            const char *mnt_path = get_vromfs_mount_path(entry);
            int mnt_path_len = mnt_path ? static_cast<int>(strlen(mnt_path)) : 0;
            if (mnt_path_len && strncmp(mnt_path, prefix.c_str(), mnt_path_len) != 0)
              return true;

            const char *pfx = prefix.c_str() + mnt_path_len;
            int pfx_len = prefix.length() - mnt_path_len;
            for (int j = 0; j < entry->files.map.size(); j++)
            {
              if (strncmp(entry->files.map[j], pfx, pfx_len) == 0 && (subdirs || !strchr(entry->files.map[j] + pfx_len, '/')))
              {
                int name_len = static_cast<int>(strlen(entry->files.map[j] + pfx_len));
                if (name_len >= suffix_len && strcmp(entry->files.map[j] + pfx_len + name_len - suffix_len, suffix.c_str()) == 0)
                {
                  if (mnt_path_len)
                    out_list.push_back() = std::string("%s%s", mnt_path, entry->files.map[j].get());
                  else
                    out_list.push_back() = entry->files.map[j];
                }
              }
            }
            return true;
          });
        }
        */
    }

    int find_files_in_folder(std::vector<std::string>& out_list, const char* dir_path, const char* file_suffix_to_match, bool vromfs, bool realfs, bool subdirs)
    {
        if (strcmp(file_suffix_to_match, "*") == 0 || strcmp(file_suffix_to_match, "*.*") == 0)
            file_suffix_to_match = "";
        else if (
            strncmp(file_suffix_to_match, "*.", 2) == 0 && !strchr(file_suffix_to_match + 2, '*') && !strchr(file_suffix_to_match + 2, '?'))
            file_suffix_to_match++;
        if (strchr(file_suffix_to_match, '*') || strchr(file_suffix_to_match, '?'))
        {
            NAU_LOG_ERROR(nau::string::format(nau::string(u8"{}: bad file_suffix_to_match=\"{}\", no wildcard matching allowed!"), nau::string(__FUNCTION__), nau::string(file_suffix_to_match)));
            return 0;
        }

        std::string tmp_dir_path;
        if (dd_resolve_named_mount(tmp_dir_path, dir_path))
            dir_path = tmp_dir_path.c_str();

        int start_cnt = out_list.size();
        // TODO: VROM integration
        // if (vromfs && vromfs_first_priority)
        //   find_vromfs_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);
        if (realfs)
            find_real_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);
        // TODO: VROM integration
        // if (vromfs && !vromfs_first_priority)
        //   find_vromfs_files_in_folder(out_list, dir_path, file_suffix_to_match, subdirs);

        // In case if realfs is true and vromfs is true
        // We might get the same paths in the list:
        // Ones froms file system and ones from vroms.
        // So, ensure that we do not return duplicated paths.
        if (vromfs && realfs)
            nau::hal::detail::remove_duplicate_file_paths(out_list);

        return out_list.size() - start_cnt;
    }

    int find_file_in_vromfs(std::vector<std::string>& out_list, const char* filename)
    {
        return 0;
        // TODO: VROM integration
        /*
        int start_cnt = out_list.size();
        std::string fname;
        if (!dd_resolve_named_mount(fname, filename))
          fname = filename;

        dd_strlwr(fname.data());
        int len = fname.length();

        iterate_vroms([&](VirtualRomFsData *entry, size_t) {
          const char *mnt_path = get_vromfs_mount_path(entry);
          for (int j = 0; j < entry->files.map.size(); j++)
          {
            const char *f = entry->files.map[j];
            int flen = (int)strlen(f);
            if (flen < len)
              continue;
            if (strcmp(f + flen - len, fname.c_str()) != 0)
              continue;
            if (mnt_path)
              out_list.push_back() = std::string("%s%s", mnt_path, entry->files.map[j].get());
            else
              out_list.push_back() = entry->files.map[j];
          }
          return true;
        });

        return out_list.size() - start_cnt;
        */
    }
}  // namespace nau::iosys
