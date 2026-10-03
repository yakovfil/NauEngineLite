// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <vector>

#include "nau/ui/elements/layer.h"
#include "nau/ui/elements/node.h"

namespace nau::test
{
    // TODO NAU-2089
    /*
    TEST(TestUINodeApi, getChildren)
    {
        nau::ui::Node* root = nau::ui::Node::create();

        {
          std::vector<nau::ui::Node*> children;
          root->getChildren(children);

          ASSERT_EQ(children.size(), 0);
        }

        {
            std::vector<nau::ui::Node*> setupChildren;
            nau::ui::Node* child1 = nau::ui::Node::create();
            nau::ui::Layer* child2 = nau::ui::Layer::create();
            setupChildren.push_back(child1);
            setupChildren.push_back(child2);
            root->addChild(child1);
            root->addChild(child2);

            std::vector<nau::ui::Node*> children;
            root->getChildren(children);

            ASSERT_EQ(children.size(), 2);

            for( const auto& child : children)
            {
                ASSERT_NE(child, nullptr);
            }
        }
    }
    */
}


