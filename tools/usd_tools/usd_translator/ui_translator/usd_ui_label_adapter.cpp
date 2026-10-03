// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "usd_ui_label_adapter.h"

#include <memory>
#include <utility>

#include "nau/NauGuiSchema/nauGuiLabel.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/ui/label.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec4d.h"
#include "usd_proxy/usd_proxy_decorators_regestry.h"

using namespace nau;
using namespace PXR_NS;

namespace UsdTranslator
{
    UsdUiLabelAdapter::UsdUiLabelAdapter(PXR_NS::UsdPrim prim)
    : UsdUiNodeAdapter(prim)
    , m_label(nullptr)
    {
    }

    bool UsdUiLabelAdapter::isValid() const
    {
        return !!m_label;
    }

    void UsdUiLabelAdapter::update()
    {
        if (!m_label)
        {
            return;
        }

        internalUpdate();
    }

    void UsdUiLabelAdapter::serializeNodeContent(nau::DataBlock &blk)
    {
        UsdUiNodeAdapter::serializeNodeContent(blk);

        std::string text;
        PXR_NS::SdfAssetPath sdfPath;

        const GuiNauGuiLabel usdLabel{ getPrim() };

        usdLabel.GetTextAttr().Get(&text);
        usdLabel.GetFontFontAttr().Get(&sdfPath);
        const auto sourcePath = getSourcePath(sdfPath);

        nau::DataBlock* labelData = blk.addBlock("label_data");
        labelData->setStr("text", text.c_str());
        labelData->setStr("font", sourcePath.c_str());
    }

    nau::Uid UsdUiLabelAdapter::getUid() const
    {
        return id;
    }

    nau::ui::Node* UsdUiLabelAdapter::initializeNode()
    {
        m_label = createLabel();
        m_node = m_label;
        m_label->retain();

        nau::getServiceProvider().get<nau::ui::UiManager>().setElementChangedCallback(id, [this](nau::ui::Node* node) 
        {
            if(id == node->getUid())
            {
                internalPrimUpdate(node);
            }
        });

        UsdUiLabelAdapter::internalUpdate();

        return m_label;
    }

    nau::ui::Node* UsdUiLabelAdapter::getNode() const
    {
        return m_label;
    }

    void UsdUiLabelAdapter::addChildInternal(nau::ui::Node* node)
    {
        m_label->addChild(node);
    }

    void UsdUiLabelAdapter::internalUpdate()
    {
        UsdUiNodeAdapter::internalUpdate();

        validateDataCache();

        if(m_cachedAdapterData.m_isDirty)
        {
            m_label->addFont(getSourcePath(m_cachedAdapterData.m_sdfFontPath));

            m_label->setOverflowType((nau::ui::NauLabel::Overflow) m_cachedAdapterData.m_overflowType);
            m_label->setWrapping((nau::ui::NauLabel::Wrapping) m_cachedAdapterData.m_wrappingype);
            m_label->setHorizontalAlignment((nau::ui::HorizontalAlignment) m_cachedAdapterData.m_horizontalAlignmentType);
            m_label->setVerticalAlignment((nau::ui::VerticalAlignment)m_cachedAdapterData.m_verticalAlignmentType);

            if(m_label->getText() != m_cachedAdapterData.m_text.c_str())
            {
                m_label->setText(m_cachedAdapterData.m_text.c_str());
            }
            else
            {
                m_label->updateLabel();
            }
        }
    }

    nau::ui::NauLabel* UsdUiLabelAdapter::createLabel()
    {
        const GuiNauGuiLabel usdLabel{ getPrim() };

        PXR_NS::SdfAssetPath sdfPath;
        usdLabel.GetFontFontAttr().Get(&sdfPath);
        const auto sourcePath = getSourcePath(sdfPath);
        auto factory = std::make_unique<nau::ui::SymbolFactory>();
        factory->registerProvider(sourcePath);

        auto label = nau::ui::NauLabel::create(std::move(factory));
        id = label->getUid();

        return label;
    }

    void UsdUiLabelAdapter::destroyNode()
    {
        m_label->removeFromParent();
        m_label->release();
    }

    void UsdUiLabelAdapter::validateDataCache()
    {
         const GuiNauGuiLabel usdLabel{ getPrim() };

        LabelAdapterData data;

        usdLabel.GetTextAttr().Get(&data.m_text);
        usdLabel.GetFontFontAttr().Get(&data.m_sdfFontPath);
        
        usdLabel.GetOverflowTypeAttr().Get(&data.m_overflowType);
        usdLabel.GetWrappingTypeAttr().Get(&data.m_wrappingype);
        usdLabel.GetHorizontalAlignmentTypeAttr().Get(&data.m_horizontalAlignmentType);
        usdLabel.GetVerticalAlignmentTypeAttr().Get(&data.m_verticalAlignmentType);

        bool isFirstInitialization = !m_cachedAdapterData.m_initialized;
        bool isDataChanged = !(m_cachedAdapterData == data);

        m_cachedAdapterData = data;
        m_cachedAdapterData.m_initialized = true;
        m_cachedAdapterData.m_isDirty = isFirstInitialization || isDataChanged;
    }

    DEFINE_UI_TRANSLATOR(UsdUiLabelAdapter, "NauGuiLabel"_tftoken);
}
