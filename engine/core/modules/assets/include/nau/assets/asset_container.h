// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>

#include "nau/assets/asset_accessor.h"
#include "nau/async/task.h"
#include "nau/io/stream.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/serialization/runtime_value.h"
#include "nau/assets/asset_content_provider.h"

namespace nau
{
    /**
     * @brief Provides an interface for granting access to the loaded assets. 
     */
    struct NAU_ABSTRACT_TYPE IAssetContainer : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IAssetContainer, IRefCounted)

        using Ptr = nau::Ptr<IAssetContainer>;

        /*
         * @brief Retrieves an implementation-defined object providing access to the asset.
         * 
         * @param [in] path Asset path within the container.
         * @return          Implementation-defined object for accessing the asset (e.g. an accessor or a view).
         */
        virtual nau::Ptr<> getAsset(eastl::string_view path = {}) = 0;

        /**
         * @brief Retrieves a collection of names of the contained assets.
         * 
         * @return A collection of contained assets names.
         */
        virtual eastl::vector<eastl::string> getContent() const = 0;
    };

    /*
     * @brief Provides an interface for loading assets into containers.
     */
    struct NAU_ABSTRACT_TYPE IAssetContainerLoader
    {
        NAU_TYPEID(nau::IAssetContainerLoader)

        /**
         * @brief Destructor.
         */
        virtual ~IAssetContainerLoader() = default;

        /**
         * @brief Retrieves a collection of asset kinds supported by the implementation.
         * 
         * @return A collection of asset kind names supported by the interface implementation.
         */
        virtual eastl::vector<eastl::string_view> getSupportedAssetKind() const = 0;

        /**
         * @brief Schedules asset load from a byte stream into a container.
         * 
         * @param [in] stream   A pointer to the byte stream to load the asset from.
         * @param [in] info     Additional asset information.
         * @return              Task object providing access to the operation status as well as the resulted asset container.
         */
        virtual async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr, AssetContentInfo info) = 0;

        /**
         * @brief Retrieves the default settings applied when loading an asset.
         * 
         * @return A pointer to the retrieved object.
         */
        virtual RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const = 0;
    };
}  // namespace nau
