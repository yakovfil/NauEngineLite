// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau_uid_resolver.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/defineResolver.h>

#include <filesystem>

#include "nau/assets/asset_db.h"
#include "nau/io/virtual_file_system.h"
#include "nau/service/service_provider.h"
#include "nau/utils/uid.h"
#include "nau_api.h"

PXR_NAMESPACE_USING_DIRECTIVE

AR_DEFINE_RESOLVER(NauUIDResolver, ArDefaultResolver);

std::string NauUIDResolver::_CreateIdentifier(const std::string& assetPath, const PXR_NS::ArResolvedPath& anchorAssetPath) const
{
    return assetPath;
}

PXR_NS::ArResolvedPath NauUIDResolver::_Resolve(const std::string& assetPath) const
{
    // TfStringStartsWith(assetPath, "uid:")
    if (auto pos = assetPath.find("uid:"); pos != std::string::npos)
    {
        auto uid_str = assetPath.substr(pos + 4);
        auto uid = nau::Uid::parseString(uid_str);
        if (uid)
        {
            auto path = nau::getServiceProvider().get<nau::IAssetDB>().getNausdPathFromUid(*uid);
            auto& vfs = nau::getServiceProvider().get<nau::io::IVirtualFileSystem>();
            auto fs_path = vfs.resolveToNativePath("/content/" + path);

            if (!fs_path.empty())
            {
                namespace fs = std::filesystem;
                return ArResolvedPath(fs::path(fs_path).string().c_str());
            }
            else if (!path.empty())
                return ArResolvedPath(path.c_str());
        }
    }

    return ArDefaultResolver::_Resolve(assetPath);
}
