// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <functional>
#include <string_view>
#include <unordered_set>

#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/utils/preprocessor.h"

#include NAU_PLATFORM_HEADER(utils/uid.h)

namespace nau::test
{
    namespace
    {
        struct DataWithUid
        {
            unsigned intValue;
            Uid uidValue;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(intValue),
                CLASS_FIELD(uidValue), )
        };
    }  // namespace

    /**
     */
    TEST(TestUid, ConstructDefault)
    {
        const Uid uid;
        ASSERT_FALSE(uid);
    }

    /**
     */
    TEST(TestUid, Generate)
    {
        const Uid uid1 = Uid::generate();
        const Uid uid2 = Uid::generate();

        ASSERT_NE(uid1, uid2);
    }

    /**
     */
    TEST(TestUid, Compare)
    {
        const Uid uid1 = Uid::generate();
        const Uid uid2 = Uid::generate();

        const bool greater = uid1 < uid2;
        const bool less = uid2 < uid1;

        ASSERT_NE(greater, less);
    }

    /**
     */
    TEST(TestUid, ToStringAndParse)
    {
        const Uid uid1 = Uid::generate();
        const auto str = toString(uid1);

        {
            const auto parseResult = Uid::parseString(str);
            ASSERT_TRUE(parseResult);
            ASSERT_EQ(uid1, *parseResult);
        }

        {
            Uid uid2;
            ASSERT_TRUE(parse(str, uid2));
            ASSERT_TRUE(uid2);
            ASSERT_EQ(uid1, uid2);
        }
    }

    /**
     */
    TEST(TestUid, FailToParse)
    {
        ASSERT_FALSE(Uid::parseString(""));
        ASSERT_FALSE(Uid::parseString("AA-BB"));
    }

    /**
     */
    TEST(TestUid, StdHash)
    {
        const Uid uid1 = Uid::generate();
        const Uid uid2 = Uid::generate();

        const Uid uid11 = uid1;

        std::hash<Uid> hash;
        ASSERT_NE(hash(uid1), hash(uid2));
        ASSERT_EQ(hash(uid1), hash(uid11));

        const Uid uidNull1;
        const Uid uidNull2;
        ASSERT_EQ(hash(uidNull1), hash(uidNull2));
    }

    /**
     */

    /**
     */
    TEST(TestUid, UseWithStdUnorderedSet)
    {
        std::unordered_set<Uid> uids;

        const auto uidExists = [&uids](const Uid& uid)
        {
            return uids.find(uid) != uids.end();
        };

        const auto [iter1, emplaceOk1] = uids.emplace(Uid::generate());
        const auto [iter2, emplaceOk2] = uids.emplace(Uid::generate());
        const auto [iter3, emplaceOk3] = uids.emplace(Uid::generate());
        const auto [iter4, emplaceOk4] = uids.emplace(Uid::generate());

        ASSERT_TRUE(emplaceOk1);
        ASSERT_TRUE(emplaceOk2);
        ASSERT_TRUE(emplaceOk3);
        ASSERT_TRUE(emplaceOk4);
        ASSERT_EQ(uids.size(), 4);

        ASSERT_TRUE(uidExists(*iter1));
        ASSERT_TRUE(uidExists(*iter2));
        ASSERT_TRUE(uidExists(*iter3));
        ASSERT_TRUE(uidExists(*iter4));
    }

    /**
     */

    /**
     */
    TEST(TestUid, StringSerialization)
    {
        using namespace serialization;
        static_assert(StringParsable<Uid>);

        DataWithUid data = {
            .intValue = 77,
            .uidValue = Uid::generate()};

        auto json = JsonUtils::stringify(data);
        auto data2 = *JsonUtils::parse<DataWithUid>(json);

        ASSERT_EQ(data.uidValue, data2.uidValue);
    }

    /**
     */
    TEST(TestUid, StringSerializationError)
    {
        using namespace serialization;
        static_assert(StringParsable<Uid>);

        std::u8string_view json =
            u8R"--(
            {
                "uidValue": "bad-string",
                "intValue": 77
            }
        )--";

        Result<DataWithUid> parseResult = JsonUtils::parse<DataWithUid>(json);
        ASSERT_FALSE(parseResult);
        ASSERT_TRUE(parseResult.getError());
        ASSERT_FALSE(parseResult.getError()->getMessage().empty());
    }

    /**
     */
    TEST(TestUid, InvalidTypeSerializationError)
    {
        using namespace serialization;
        static_assert(StringParsable<Uid>);

        std::u8string_view json =
            u8R"--(
            {
                "uidValue": 11223344,
                "intValue": 77
            }
        )--";

        Result<DataWithUid> parseResult = JsonUtils::parse<DataWithUid>(json);
        ASSERT_FALSE(parseResult);
        ASSERT_TRUE(parseResult.getError());
        ASSERT_FALSE(parseResult.getError()->getMessage().empty());
    }
}  // namespace nau::test
