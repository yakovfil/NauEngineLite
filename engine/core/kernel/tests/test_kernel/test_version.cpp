// test_messaging.cpp
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//

#include "nau/version/engine_version.h"

namespace nau::test
{
    TEST(TestVersion, EngineVersionCreate)
    {
        EngineVersion ev(1, 2, 3, "commit", "branch");
        ASSERT_EQ(ev.getMajor(), 1);
        ASSERT_EQ(ev.getMinor(), 2);
        ASSERT_EQ(ev.getPatch(), 3);
        ASSERT_EQ(ev.getCommit(), "commit");
        ASSERT_EQ(ev.getBranch(), "branch");
    }

    TEST(TestVersion, EngineVersionMatch)
    {
        EngineVersion ev(1, 2, 3);
        EngineVersion ev_match(1, 2, 3);
        EngineVersion ev_notmatch(4, 5, 6);
        ASSERT_TRUE(ev.matchVersion(ev_match));
        ASSERT_FALSE(ev.matchVersion(ev_notmatch));
    }

    TEST(TestVersion, EngineVersionMatchBuild)
    {
        EngineVersion ev(1, 2, 3, "commit", "branch");
        EngineVersion ev_match(1, 2, 3, "commit", "branch");
        EngineVersion ev_notmatch(1, 2, 3, "othercommit", "otherbranch");
        ASSERT_TRUE(ev.matchVersionAndBuild(ev_match));
        ASSERT_FALSE(ev.matchVersionAndBuild(ev_notmatch));
    }

    TEST(TestVersion, EngineVersionMatchCurrent)
    {
        EngineVersion ev(NAU_VERSION_MAJOR, NAU_VERSION_MINOR, NAU_VERSION_PATCH, NAU_GIT_COMMIT, NAU_GIT_BRANCH);
        ASSERT_TRUE(ev.matchVersionAndBuild(EngineVersion::current()));
    }

    TEST(TestVersion, EngineVersionCompare)
    {
        EngineVersion ev(1, 2, 3);
        EngineVersion ev_lesser_major(0, 2, 3);
        EngineVersion ev_lesser_minor(1, 1, 3);
        EngineVersion ev_lesser_patch(1, 2, 2);
        EngineVersion ev_greater_major(2, 2, 3);
        EngineVersion ev_greater_minor(1, 3, 3);
        EngineVersion ev_greater_patch(1, 2, 4);

        ASSERT_TRUE(ev.greaterOrEqualVersion(ev_lesser_major));
        ASSERT_TRUE(ev.greaterOrEqualVersion(ev_lesser_minor));
        ASSERT_TRUE(ev.greaterOrEqualVersion(ev_lesser_patch));

        ASSERT_FALSE(ev.greaterOrEqualVersion(ev_greater_major));
        ASSERT_FALSE(ev.greaterOrEqualVersion(ev_greater_minor));
        ASSERT_FALSE(ev.greaterOrEqualVersion(ev_greater_patch));
    }

    TEST(TestVersion, EngineVersionString)
    {
        EngineVersion ev_base(1, 2, 3);
        EngineVersion ev_vcs(1, 2, 3, "commit", "branch");
        ASSERT_EQ(ev_base.toString(), "1.2.3");
        ASSERT_EQ(ev_vcs.toString(), "1.2.3-commit+branch");
    }

    TEST(TestVersion, EngineVersionParse)
    {
        EngineVersion ev_base;
        bool result_base = EngineVersion::parse("1.2.3", ev_base);
        ASSERT_TRUE(result_base);
        ASSERT_EQ(ev_base.toString(), "1.2.3");

        EngineVersion ev_vcs;
        bool result_vcs = EngineVersion::parse("1.2.3-commit+branch", ev_vcs);
        ASSERT_TRUE(result_vcs);
        ASSERT_EQ(ev_vcs.toString(), "1.2.3-commit+branch");
    }
}  // namespace nau::test
