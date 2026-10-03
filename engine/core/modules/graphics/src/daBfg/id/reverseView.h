// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <iterator>
#include <utility>

template <class Container>
class ReverseView
{
public:
    template <class T>
    ReverseView(T&& cont) :
        container_{std::forward<T>(cont)}
    {
    }

    auto begin() const
    {
        return std::reverse_iterator(std::end(container_));
    }
    auto end() const
    {
        return std::reverse_iterator(std::begin(container_));
    }

private:
  Container container_;
};

// For lvalue, we want a reference to the original container,
// while for an rvalue we actually want to store it inside the view.
template <class T>
ReverseView(T &&) -> ReverseView<T>;
