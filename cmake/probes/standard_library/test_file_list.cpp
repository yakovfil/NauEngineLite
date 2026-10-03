// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>
#include <cstdlib>

#include "file_list_utils.h"

namespace
{
    void check(std::vector<std::string> paths, const std::vector<std::string>& expected)
    {
        nau::hal::detail::remove_duplicate_file_paths(paths);
        if (paths != expected)
        {
            std::fputs("File list did not preserve the sorted unique paths\n", stderr);
            std::abort();
        }
        nau::hal::detail::remove_duplicate_file_paths(paths);
        if (paths != expected)
        {
            std::fputs("Duplicate removal was not idempotent\n", stderr);
            std::abort();
        }
    }
}  // namespace

int main()
{
    check({}, {});
    check({"one"}, {"one"});
    check({"c", "a", "b"}, {"a", "b", "c"});
    check({"a", "a", "a", "b"}, {"a", "b"});
    check({"same", "same", "same"}, {"same"});
    check({"b", "a", "c", "b", "a", "c"}, {"a", "b", "c"});
    check({"path", "Path", "path"}, {"Path", "path"});
    check({"", "", "a", "a"}, {"", "a"});
    const std::string longPath(512, 'x');
    check({longPath, "short", longPath}, {"short", longPath});
}
