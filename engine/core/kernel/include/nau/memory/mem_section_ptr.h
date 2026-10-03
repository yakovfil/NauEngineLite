// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

/**
 * @file mem_section_ptr.h
 * @brief Defines the MemSectionPtr class
 */
#pragma once
#include <string>

#include "nau/memory/mem_section.h"

namespace nau
{
    /**
     * @brief A wrapper for memory sections.
     */
    class NAU_KERNEL_EXPORT MemSectionPtr final
    {
    public:
        MemSectionPtr() = default;
        MemSectionPtr(const std::string kind, MemSection* ptr);
        ~MemSectionPtr();
        MemSectionPtr(const MemSectionPtr&) = delete;
        MemSectionPtr& operator=(const MemSectionPtr&) = delete;
        MemSectionPtr(MemSectionPtr&& val);
        MemSectionPtr& operator=(MemSectionPtr&& val);

        /**
         * @brief Dereferences the pointer to access the memory section.
         *
         * @return A pointer to the memory section.
         */
        MemSection* operator->() const;

        /**
         * @brief Gets the raw pointer to the memory section.
         *
         * @return A pointer to the memory section.
         */
        [[nodiscard]] MemSection* get() const;

        /**
         * @brief Gets the kind of the memory section.
         *
         * @return A reference to the string representing the kind of the memory section.
         */
        [[nodiscard]] const std::string& getKind() const;

        /**
         * @brief Checks if the memory section pointer is valid.
         *
         * @return true if the pointer is valid, false otherwise.
         */
        [[nodiscard]] bool valid() const;

    private:
        MemSection* m_ptr = nullptr;
        std::string m_kind;
    };

}  // namespace nau