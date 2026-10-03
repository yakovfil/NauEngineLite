// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <compare>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/kernel/kernel_config.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"
#include "nau/utils/result.h"

/**
 * @brief Provides the `FsPath` class for representing and manipulating file system paths.
 */

namespace nau::io_detail
{
    // template <typename, typename = void>
    // inline constexpr bool IsPathSource = false;

    // template<typename T>
    // inline constexpr bool IsPathSource<T, void_t<typename std::iterator_traits<T>::value_type

    /**
     * @brief Converts various source types to a `std::basic_string` representation of a file path.
     * @tparam C Character type (e.g., `char`, `wchar_t`).
     * @tparam Len Size of the character array.
     * @param str Null-terminated character array representing the file path.
     * @return `std::basic_string` representation of the file path.
     */
    template <typename C, size_t Len>
    std::basic_string<C> fsPathFromSource(const C (&str)[Len])
    {  // check that source is null terminated ?
        if (str[Len - 1] == 0)
        {
            return {str, Len - 1};
        }

        return {str, Len};
    }

    /**
     * @brief Converts a `std::basic_string_view` to a `std::basic_string` representation of a file path.
     * @tparam C Character type (e.g., `char`, `wchar_t`).
     * @param str `std::basic_string_view` representing the file path.
     * @return `std::basic_string` representation of the file path.
     */
    template <typename C>
    std::basic_string<C> fsPathFromSource(std::basic_string_view<C> str)
    {
        return {str.data(), str.size()};
    }

    /**
     * @brief Returns a const reference to a `std::basic_string` representing the file path.
     * @tparam C Character type (e.g., `char`, `wchar_t`).
     * @param str `std::basic_string` representing the file path.
     * @return A const reference to the `std::basic_string` representing the file path.
     */
    template <typename C>
    const std::basic_string<C>& fsPathFromSource(const std::basic_string<C>& str)
    {
        return (str);
    }

    /**
     * @brief Converts `std::string_view` to a `std::string` representation of a file path.
     * @param str `std::string_view` representing the file path.
     * @return `std::string` representation of the file path.
     */
    inline std::string fsPathFromSource(const std::string_view str)
    {
        return {str.data(), str.size()};
    }

    /**
     * @brief Converts `std::u8string_view` to a `std::string` representation of a file path.
     * @param str `std::u8string_view` representing the file path.
     * @return `std::string` representation of the file path.
     */
    inline std::string fsPathFromSource(const std::u8string_view str)
    {
        return {reinterpret_cast<const char*>(str.data()), str.size()};
    }

    inline std::string fsPathFromSource(const std::wstring_view str)
    {
        const std::u8string utfStr = strings::wstringToUtf8(std::wstring_view{str.data(), str.size()});

        return std::string{reinterpret_cast<const char*>(utfStr.data()), utfStr.size()};
    }

    /**
     * @brief Concept to check if a type can be used as a path source.
     * @tparam T Type to check.
     */
    template <typename T>
    concept IsPathSource = requires(const T& str) {
        fsPathFromSource(str);
    };
}  // namespace nau::io_detail

namespace nau::io
{

    NAU_KERNEL_EXPORT std::string makePreferredPathString(std::string_view pathString);
    NAU_KERNEL_EXPORT void makePreferredPathStringInplace(std::string& pathString);

    /**
     * @class FsPath
     * @brief Represents and manipulates file system paths.
     *
     * The `FsPath` class provides various methods for manipulating and querying file system paths.
     */
    class NAU_KERNEL_EXPORT FsPath
    {
    public:
        using CharType = char;                           ///< Type of characters used in the path.
        using StringType = std::basic_string<CharType>;  ///< Type used for storing the path string.

        /**
         * @brief Default constructor.
         */
        FsPath();
        /**
         * @brief Copy constructor.
         * @param other The `FsPath` object to copy.
         */
        FsPath(const FsPath&);

        /**
         * @brief Move constructor.
         * @param other The `FsPath` object to move.
         */
        FsPath(FsPath&&);

        /**
         * @brief Constructor that initializes from an rvalue string.
         * @param path Rvalue string representing the path.
         */
        FsPath(StringType&&);

        /**
         * @brief Constructs an `FsPath` from various types of path sources.
         * @tparam Str Type of the path source.
         * @param str The path source.
         */
        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        FsPath(const Str& str) :
            m_path(io_detail::fsPathFromSource(str))
        {
            makePreferredPathStringInplace(m_path);
        }

        /**
         * @brief Copy assignment operator.
         * @param other The `FsPath` object to copy.
         * @return A reference to this object.
         */
        FsPath& operator=(const FsPath&);

        /**
         * @brief Move assignment operator.
         * @param other The `FsPath` object to move.
         * @return A reference to this object.
         */
        FsPath& operator=(FsPath&&);

        /**
         * @brief Equality operator.
         * @param other The `FsPath` object to compare with.
         * @return `true` if the paths are equal, `false` otherwise.
         */
        bool operator==(const FsPath& other) const;

        /**
         * @brief Appends another path to this path.
         * @param otherPath The path to append.
         * @return A reference to this object.
         */
        FsPath& append(const FsPath& otherPath);

        /**
         * @brief Appends a string to this path.
         * @tparam Str Type of the path source.
         * @param str The string to append.
         * @return A reference to this object.
         */
        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        FsPath& append(const Str& str)
        {
            return appendInternal(io_detail::fsPathFromSource(str));
        }

        /**
         * @brief Concatenates a string to this path.
         * @tparam Str Type of the path source.
         * @param str The string to concatenate.
         * @return A reference to this object.
         */
        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        FsPath& concat(const Str& str)
        {
            return concatInternal(io_detail::fsPathFromSource(str));
        }

        /**
         * @brief Splits the path into its elements.
         * @return A sequence of strings representing the path elements.
         */
        auto splitElements() const
        {
            return strings::split(std::string_view{m_path}, std::string_view{"/"});
        }

        /**
         * @brief Replaces the extension with replacement.
         * @return The path with the extension changed.
         */
        FsPath replace_extension(const FsPath& replacement) const;

        /**
         * @brief Gets the relative path from a base path.
         * @param basePath The base path to calculate the relative path from.
         * @return The relative path.
         */
        FsPath getRelativePath(const FsPath& basePath) const;

        /**
         * @brief Gets the root path from a base path.
         * @return The root path prefix.
         */

        std::string getRootPath() const;

        /**
         * @brief Gets the parent path.
         * @return The parent path.
         */
        FsPath getParentPath() const;

        /**
         * @brief Gets the name of the file or directory.
         * @return The name of the file or directory.
         */
        std::string_view getName() const;

        /**
         * @brief Gets the extension of the file.
         * @return The extension of the file.
         */
        std::string_view getExtension() const;

        /**
         * @brief Gets the stem of the file (the name without extension).
         * @return The stem of the file.
         */
        std::string_view getStem() const;

        /**
         * @brief Gets the path as a UTF-8 string.
         * @return The UTF-8 string representation of the path.
         */
        std::string getString() const;

        /**
         * @brief Gets the path as a UTF-8 C-string.
         * @return The UTF-8 C-string representation of the path.
         */
        const char* getCStr() const;

        /**
         * @brief Checks if the path is absolute.
         * @return `true` if the path is absolute, `false` otherwise.
         */
        bool isAbsolute() const;

        /**
         * @brief Checks if the path is relative.
         * @return `true` if the path is relative, `false` otherwise.
         */
        bool isRelative() const;

        /**
         * @brief Checks if the path is empty.
         * @return `true` if the path is empty, `false` otherwise.
         */
        bool isEmpty() const;

        FsPath& makeAbsolute();

        /**
         * @brief Gets the hash code of the path.
         * @return The hash code of the path.
         */
        size_t getHashCode() const;

    private:
        FsPath& appendInternal(std::string_view str);
        FsPath& concatInternal(std::string_view str);

        friend FsPath& operator/=(FsPath& path, const FsPath& otherPath)
        {
            return path.append(otherPath);
        }

        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        friend FsPath& operator/=(FsPath& path, const Str& str)
        {
            return path.append(str);
        }

        friend FsPath operator/(const FsPath& path, const FsPath& otherPath)
        {
            return FsPath{path}.append(otherPath);
        }

        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        friend FsPath operator/(const FsPath& path, const Str& str)
        {
            return FsPath{path}.append(str);
        }

        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        friend FsPath& operator+=(FsPath& path, const Str& str)
        {
            return path.concat(str);
        }

        template <typename Str>
        requires(io_detail::IsPathSource<Str>)
        friend FsPath operator+(const FsPath& path, const Str& str)
        {
            return FsPath{path}.concat(str);
        }

        friend std::strong_ordering operator<=>(const FsPath& path1, const FsPath& path2)
        {
            return path1.m_path <=> path2.m_path;
        }

        NAU_KERNEL_EXPORT friend Result<> parse(std::string_view str, FsPath&);
        NAU_KERNEL_EXPORT friend std::string toString(const FsPath& uid);

        StringType m_path;  ///< The string representing the path.
    };

}  // namespace nau::io

// namespace eastl

namespace std
{
    /**
     * @brief Specialization of the `hash` struct for `nau::io::FsPath`.
     */
    template <>
    struct hash<::nau::io::FsPath>
    {
        /**
         * @brief Computes the hash code of the given `FsPath` object.
         * @param val The `FsPath` object.
         * @return The hash code of the `FsPath` object.
         */
        [[nodiscard]]
        size_t operator()(const ::nau::io::FsPath& val) const
        {
            return val.getHashCode();
        }
    };

}  // namespace std
