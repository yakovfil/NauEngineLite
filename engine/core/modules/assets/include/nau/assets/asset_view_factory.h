// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <vector>

#include "nau/assets/asset_accessor.h"
#include "nau/assets/asset_view.h"
#include "nau/async/task.h"
#include "nau/rtti/type_info.h"

namespace nau
{
    /**
     * @brief Provides an interface for creating asset views.
     * 
     * See also: IAssetView.
     */
    struct NAU_ABSTRACT_TYPE IAssetViewFactory : virtual IRttiObject
    {
        NAU_INTERFACE(nau::IAssetViewFactory, IRttiObject)

        /**
         * @brief Retrieves a collection of all available asset view types for this factory.
         * 
         * @return A collection of all available asset view types.
         */
        virtual std::vector<const rtti::TypeInfo*> getAssetViewTypes() const = 0;

        /**
         * @brief Schedules asset view creation operation.
         * 
         * @param [in] accessor A pointer to the object (depending on the asset type) containing the asset data.
         * @param [in] viewType Type of the asset to create the view of.
         * @return              Task object providing the operation status as well as access to the created asset view.
         */
        virtual async::Task<IAssetView::Ptr> createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType) = 0;
    };
}  // namespace nau
