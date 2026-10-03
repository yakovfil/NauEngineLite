// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_skeleton_animation_adapter.h"

#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/cache.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/skeletonQuery.h>

#include <string>
#include <vector>

#include "nau/NauAnimationClipAsset/nauAnimationSkeleton.h"
#include "nau/animation/assets/skeleton_asset.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/shared/file_system.h"
#include "usd_translator/usd_prim_translator.h"
#include "usd_translator/usd_stage_translator.h"

namespace UsdTranslator
{
    std::vector<std::string> UsdAnimationSkelAccessor::getOzzAnimationPaths(const pxr::UsdPrim& skelPrim, const std::string& assetPath)
    {
		constexpr std::string_view OZZ_FILE_EXT = ".ozz";

		pxr::UsdSkelRoot skelRoot{ skelPrim };
		if (!skelRoot)
		{
			return {};
		}

		pxr::UsdSkelCache skelCache;
		skelCache.Populate(skelRoot, pxr::UsdTraverseInstanceProxies());

		std::vector<pxr::UsdSkelBinding> bindings;
		skelCache.ComputeSkelBindings(skelRoot, &bindings, pxr::UsdTraverseInstanceProxies());
		if (bindings.empty())
		{
			return {};
		}

		auto nameEnd = std::find(assetPath.rbegin(), assetPath.rend(), '.');
		const size_t pathStartIndex = assetPath.find("/content/") + 1ull;
		const size_t pathSize = nameEnd.base() - assetPath.begin() - pathStartIndex - 1ull;
		std::string ozzAssetPath = assetPath.substr(pathStartIndex, pathSize);

        std::vector<std::string> result;
        result.reserve(bindings.size());

		for (const auto& binding : bindings)
		{
			const std::string& name = binding.GetSkeleton().GetPrim().GetName().GetString();
			ozzAssetPath.resize(pathSize);
			ozzAssetPath.reserve(pathSize + name.size() + OZZ_FILE_EXT.size());
			((ozzAssetPath += "/") += name) += OZZ_FILE_EXT;
			result.emplace_back(ozzAssetPath.c_str());
		}

		return result;
	}

    std::string UsdAnimationSkelAccessor::getOzzSkeletonPath(const std::string& assetPath)
    {
		std::string_view assetPathView = assetPath;
		auto nameStart = std::find_if(assetPathView.rbegin(), assetPathView.rend(), [](char c)
		{
			return c == '/' || c == '\\';
		});
		const size_t pathStartIndex = 0;// assetPath.find("/content/") + 1ull;
		auto nameEnd = std::find(assetPathView.rbegin(), assetPathView.rend(), '.');
		const std::string_view assetName = assetPathView.substr(nameStart.base() - assetPathView.begin(), (nameStart - nameEnd) - 1ull);
		const std::string skeletonPath = ((assetPath.substr(pathStartIndex, (nameEnd.base() - assetPathView.begin()) - pathStartIndex - 1ull) += "/") += assetName) += ".ozz";
		return skeletonPath.c_str();
	}

	UsdAnimationSkelAccessor::UsdAnimationSkelAccessor(const pxr::UsdPrim& prim, const std::string& assetPath) :
		m_skelPrim(prim)
	{
		reset(assetPath);
	}

	void UsdAnimationSkelAccessor::reset(const std::string& assetPath)
	{
		m_skeletonAssetPath = std::move(getOzzSkeletonPath(assetPath));
		m_skeletonDesc.skeletonPath = m_skeletonAssetPath;
		m_bindMatrixList.clear();

		pxr::UsdSkelRoot skelRoot{ m_skelPrim };
		if (!skelRoot)
		{
			return;
		}

		pxr::UsdSkelCache skelCache;
		skelCache.Populate(skelRoot, pxr::UsdTraverseInstanceProxies());

		std::vector<pxr::UsdSkelBinding> bindings;
		skelCache.ComputeSkelBindings(skelRoot, &bindings, pxr::UsdTraverseInstanceProxies());
		if (bindings.empty())
		{
			return;
		}

		pxr::VtArray<pxr::GfMatrix4d> matrixList;
		bindings.front().GetSkeleton().GetBindTransformsAttr().Get(&matrixList);
		m_skeletonDesc.jointsCount = static_cast<unsigned>(matrixList.size());

		m_bindMatrixList.reserve(matrixList.size());
		for (const auto& matrix : matrixList)
		{
			auto inverseMatrix = matrix.GetInverse();
			const auto row0 = inverseMatrix.GetRow(0);
			const auto row1 = inverseMatrix.GetRow(1);
			const auto row2 = inverseMatrix.GetRow(2);
			const auto row3 = inverseMatrix.GetRow(3);
			m_bindMatrixList.emplace_back(
				nau::math::vec4(row0[0], row0[1], row0[2], row0[3]),
				nau::math::vec4(row1[0], row1[1], row1[2], row1[3]),
				nau::math::vec4(row2[0], row2[1], row2[2], row2[3]),
				nau::math::vec4(row3[0], row3[1], row3[2], row3[3])
			);
		}
	}

	nau::SkeletonDataDescriptor UsdAnimationSkelAccessor::getDescriptor() const
	{
		return m_skeletonDesc;
	}

    void UsdAnimationSkelAccessor::copyInverseBindMatrices(std::vector<nau::math::mat4>& data) const
    {
		data.clear();
		data.reserve(m_bindMatrixList.size());
		std::for_each(m_bindMatrixList.begin(), m_bindMatrixList.end(), [&data](const auto& matrix)
		{
			data.emplace_back(matrix);
		});
	}


	SkeletonAnimationAdapter::SkeletonAnimationAdapter(pxr::UsdPrim prim)
		: IPrimAdapter(prim)
		, m_typeName("SkeletonAnimationAdapter")
	{
	}

	std::string_view SkeletonAnimationAdapter::getType() const
	{
		return m_typeName;
	}

	nau::async::Task<nau::scene::ObjectWeakRef<nau::scene::SceneObject>> SkeletonAnimationAdapter::initializeSceneObject(nau::scene::ObjectWeakRef<nau::scene::SceneObject> dest)
	{
		m_obj = nau::scene::ObjectWeakRef(dest);
		if (auto* component = m_obj->findFirstComponent<nau::SkeletonComponent>())
		{
			m_skeleton = component;
		}
		else
		{
			m_skeleton = co_await m_obj->addComponentAsync<nau::SkeletonComponent>();
		}

		co_await update();

		co_return m_obj;
	}

	nau::scene::ObjectWeakRef<nau::scene::SceneObject> SkeletonAnimationAdapter::getSceneObject() const
	{
		return m_obj;
	}

	nau::async::Task<> SkeletonAnimationAdapter::update()
	{
		if (!isValid())
		{
			co_return;
		}
		pxr::UsdNauAnimationSkeleton skeleton{ getPrim() };
		if (!m_skeleton || !skeleton)
		{
			co_return;
		}
		pxr::SdfAssetPath assetPath;
		skeleton.GetAnimationAssetAttr().Get(&assetPath);
		if (assetPath.GetAssetPath().empty())
		{
			co_return;
		}
		auto&& assetStage = pxr::UsdStage::Open(assetPath.GetAssetPath());
		if (assetStage == nullptr)
		{
			co_return;
		}
		auto&& primRange = assetStage->Traverse();
		auto&& primIt = std::find_if(primRange.begin(), primRange.end(), [](const pxr::UsdPrim& prim) noexcept
		{
			return prim.IsA<pxr::UsdSkelRoot>();
		});
		if (primIt == primRange.end())
		{
			co_return;
		}

		const std::string relativeAssetPath = nau::FileSystemExtensions::getRelativeAssetPath(assetPath.GetAssetPath(), false).string();
		auto skeletonAsset = nau::SkeletonAssetRef
		{ 
			nau::strings::toStringView(std::format("asset:/content/{}+[skeleton]", relativeAssetPath)),
			true
		};

		//auto loadedSkeleton = co_await skeletonAsset.getAssetViewTyped<nau::SkeletonAssetView>();

		m_skeleton->setSkeletonAsset(std::move(skeletonAsset));

		//auto&& accessor = nau::rtti::createInstance<UsdAnimationSkelAccessor>(*primIt, assetPath.GetAssetPath());
		//auto&& assetView = co_await nau::SkeletonAssetView::createFromAssetAccessor(std::move(accessor));
		//m_skeleton->setSkeletonAssetView(std::move(assetView));
	}

	bool SkeletonAnimationAdapter::isValid() const
	{
		return !!m_obj;
	}

	void SkeletonAnimationAdapter::destroySceneObject()
	{
		m_obj = nullptr;
	}

	[[maybe_unused]] DEFINE_TRANSLATOR(SkeletonAnimationAdapter, "AnimationSkeleton"_tftoken);
}
