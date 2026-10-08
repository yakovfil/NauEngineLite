// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "nau/diag/device_error.h"
#include "nau/module/module.h"
#include "nau/module/module_manager.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/serialization/json_utils.h"
#include "nau/string/format.h"
#include "nau/string/hash_string.h"
#include "nau/string/string_conv.h"

namespace
{
    class ConversionDiagnostic final : public nau::diag::IDeviceError
    {
        NAU_RTTI_CLASS(ConversionDiagnostic, nau::diag::IDeviceError)
    public:
        unsigned count = 0;
        nau::diag::FailureActionFlag handleFailure(const nau::diag::FailureData&) override
        {
            ++count;
            return {};
        }
    };
}  // namespace

TEST(StringMigration, RecordedValues)
{
    const std::string bytes("a\0b", 3);
    const auto unicode = nau::strings::toUtf8(u8"A\u0416\U0001f680");
    EXPECT_EQ(nau::utils::format("[{}]", unicode), "[A\xd0\x96\xf0\x9f\x9a\x80]");
    EXPECT_EQ(nau::utils::format("{}", bytes), bytes);
    EXPECT_EQ(nau::serialization::JsonUtils::stringify(std::vector<std::string>{"alpha", bytes, "omega"}), u8"[\"alpha\",\"a\\u0000b\",\"omega\"]");
    constexpr size_t abc = sizeof(size_t) == 8 ? static_cast<size_t>(0xe71fa2190541574bull) : size_t{0x1a47e90b};
    constexpr size_t a = sizeof(size_t) == 8 ? static_cast<size_t>(0xaf63dc4c8601ec8cull) : size_t{0xe40c292c};
    EXPECT_EQ(nau::hash_string(std::string("abc")).toHash(), abc);
    constexpr size_t unicodeHash = sizeof(size_t) == 8 ? static_cast<size_t>(0xbb2a0c587af258e5ull) : size_t{0xaa920425};
    EXPECT_EQ(nau::hash_string(unicode).toHash(), unicodeHash);
    EXPECT_EQ(nau::hash_string(bytes).toHash(), a);
    EXPECT_EQ(nau::strings::codePointCount(unicode), 3u);
    EXPECT_EQ(unicode.size(), 7u);
}

TEST(StringMigration, ExplicitUnicodeOperations)
{
    using namespace nau::strings;
    const auto text = toUtf8(u8"A\u0416\u4f60\U0001f680");
    EXPECT_EQ(text.size(), 10u);
    EXPECT_EQ(codePointCount(text), 4u);
    EXPECT_EQ(codePointSlice(text, 1, 2), toUtf8(u8"\u0416\u4f60"));
    EXPECT_TRUE(codePointSlice(text, 4).empty());
    EXPECT_EQ(toUtf8(utf8ToUtf16(text)), text);
    auto decoded = utf8ToUtf32(text);
    EXPECT_EQ(decoded, U"A\u0416\u4f60\U0001f680");
    decoded[1] = U'\U0001f600';
    EXPECT_EQ(toUtf8(decoded), toUtf8(u8"A\U0001f600\u4f60\U0001f680"));
    const std::string nulls("a\0b", 3);
    EXPECT_EQ(toUtf8(utf8ToUtf16(nulls)), nulls);
    EXPECT_EQ(toUtf8(utf8ToUtf32(nulls)), nulls);
    EXPECT_TRUE(toUtf8(static_cast<const char*>(nullptr)).empty());
    EXPECT_TRUE(toUtf8(static_cast<const char16_t*>(nullptr)).empty());
    EXPECT_FALSE(isValidUtf8(std::string("\xc0\xaf", 2)));
    EXPECT_TRUE(isValidUtf8({}));
}

TEST(StringMigration, InvalidConversionsPublishNoPrefix)
{
    auto diagnostic = std::make_unique<ConversionDiagnostic>();
    auto* observed = diagnostic.get();
    nau::diag::IDeviceError::Ptr previous;
    nau::diag::setDeviceError(std::move(diagnostic), &previous);
    EXPECT_TRUE(nau::strings::toUtf8(std::string_view{"A\xc0\xaf"}).empty());
    EXPECT_TRUE(nau::strings::toUtf8(std::u16string{u'A', 0xd800}).empty());
    EXPECT_TRUE(nau::strings::toUtf8(std::u32string{U'A', 0x110000}).empty());
    EXPECT_TRUE(nau::strings::utf8ToUtf16("A\xc0\xaf").empty());
    EXPECT_TRUE(nau::strings::utf8ToUtf32("A\xc0\xaf").empty());
    EXPECT_EQ(observed->count, 5u);
    nau::diag::setDeviceError(std::move(previous));
}

TEST(StringMigration, UnicodeModuleIdentity)
{
    struct Module final : nau::DefaultModuleImpl
    {
        std::string getModuleName() override
        {
            return nau::strings::toUtf8(u8"Module\u0416\U0001f680");
        }
    };
    auto manager = nau::createModuleManager();
    auto module = std::make_shared<Module>();
    const auto name = module->getModuleName();
    manager->registerModule(name.c_str(), module);
    EXPECT_TRUE(manager->isModuleLoaded(name.c_str()));
    EXPECT_EQ(manager->getModuleInitialized(nau::hash_string{name})->getModuleName(), name);
}
