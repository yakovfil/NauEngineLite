// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau_usd_file_format.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/usdFileFormat.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdNauUsdFileFormatTokens, USD_NAUUSD_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION_WITH_TAG(TfType, UsdNauUsdFileFormat)
{
    SDF_DEFINE_FILE_FORMAT(UsdNauUsdFileFormat, SdfTextFileFormat);
}

UsdNauUsdFileFormat::UsdNauUsdFileFormat() :
    SdfTextFileFormat(UsdNauUsdFileFormatTokens->Id,
                      UsdNauUsdFileFormatTokens->Version,
                      UsdUsdFileFormatTokens->Target)
{
    // Do Nothing.
}

UsdNauUsdFileFormat::~UsdNauUsdFileFormat()
{
    // Do Nothing.
}

PXR_NAMESPACE_CLOSE_SCOPE
