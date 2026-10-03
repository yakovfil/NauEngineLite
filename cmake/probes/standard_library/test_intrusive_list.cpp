// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "nau/utils/intrusive_list.h"

void check(bool condition)
{
    if (!condition)
        std::abort();
}
struct Node : nau::IntrusiveListNode
{
    int value = 0;
    static inline int destroyed = 0;
    ~Node()
    {
        ++destroyed;
    }
};

int main()
{
    static_assert(std::bidirectional_iterator<nau::IntrusiveList<Node>::iterator>);
    static_assert(std::bidirectional_iterator<nau::IntrusiveList<Node>::const_iterator>);
    Node first, second, third;
    first.value = 1;
    second.value = 2;
    third.value = 3;
    {
        nau::IntrusiveList<Node> list;
        list.push_back(second);
        list.push_front(first);
        list.insert(list.end(), third);
        check(list.size() == 3 && &list.front() == &first && &list.back() == &third);
        const auto& constant = list;
        check(std::find_if(constant.begin(), constant.end(), [](auto& node)
        {
            return node.value == 2;
        })->value == 2);
        auto after = list.erase(list.locate(second));
        check(&*after == &third && !list.contains(second));
        check(second.next == nullptr && second.previous == nullptr);
        list.push_back(second);
        list.remove(first);
        check(list.size() == 2 && &list.back() == &second);
        list.clear();
        check(list.empty() && Node::destroyed == 0);
        list.push_back(first);
    }
    check(first.next == nullptr && first.previous == nullptr && Node::destroyed == 0);
    std::puts("intrusive list: stable membership, unlink, reuse and non-ownership passed");
}
