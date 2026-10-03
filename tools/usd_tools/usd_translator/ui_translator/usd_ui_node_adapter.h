// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "../usd_prim_translator.h"
#include "usd_ui_prim_adapter.h"

#include "usd_ui_node_adapter.h"

#include "nau/service/service_provider.h"
#include "nau/ui.h"

namespace nau::ui
{
    class Node;
}

namespace UsdTranslator
{
    class UsdUiNodeAdapter : public IUIPrimAdapter
    {
    public:
        UsdUiNodeAdapter(PXR_NS::UsdPrim prim);

        bool isValid() const override;
        void update() override;

        nau::Uid getUid() const override;

        virtual nau::ui::Node* initializeNode() override;
        virtual nau::ui::Node* getNode() const override;

        void addChildInternal(nau::ui::Node* node) override;

        virtual void serializeNodeContent(nau::DataBlock &blk) override;
        virtual std::string getType() const override {return  "node"; }

    private:
        nau::ui::Node* createNode();

    protected:
        void destroyNode() override;

        virtual void internalUpdate();
        virtual void internalPrimUpdate(nau::ui::Node* node);


    protected:
        nau::ui::Node* m_node = nullptr;

    private:
        nau::Uid id;
    };
}
