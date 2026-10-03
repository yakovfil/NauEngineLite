// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <filesystem>
#include <string_view>

#include "nau/io/fs_path.h"
#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau::test
{
    using namespace ::testing;
    using namespace nau::io;

    /**
     */
    TEST(TestFsPath, EmptyByDefault)
    {
        FsPath path;
        ASSERT_TRUE(path.isEmpty());
        ASSERT_FALSE(path.isAbsolute());
        ASSERT_FALSE(path.isRelative());
    }

    /**
     */
    TEST(TestFsPath, MakePreferredPathString)
    {
        // handle empty string
        ASSERT_TRUE(io::makePreferredPathString("").empty());

        // handle whitespace only string
        ASSERT_TRUE(io::makePreferredPathString("  ").empty());

        // root only
        ASSERT_EQ(io::makePreferredPathString("/"), "/");
        ASSERT_EQ(io::makePreferredPathString("   //  "), "/");
        ASSERT_EQ(io::makePreferredPathString("   ////  "), "/");
        ASSERT_EQ(io::makePreferredPathString("   \\  "), "/");

        // no modification
        ASSERT_EQ(io::makePreferredPathString("/path1/path2"), "/path1/path2");

        // no modification (no root)
        ASSERT_EQ(io::makePreferredPathString("path1/path2"), "path1/path2");

        // remove trailing (unused) slash
        ASSERT_EQ(io::makePreferredPathString("path1/path2/"), "path1/path2");

        // eliminate double (empty) slashes
        ASSERT_EQ(io::makePreferredPathString("/path1//path2"), "/path1/path2");

        // eliminate more double (empty) slashes
        ASSERT_EQ(io::makePreferredPathString("//path1////path2//"), "/path1/path2");

        // eliminate leading and trailing spaces
        ASSERT_EQ(io::makePreferredPathString("   //path1////path2//   "), "/path1/path2");

        // converting backslashes
        ASSERT_EQ(io::makePreferredPathString("\\path1/path2\\\\path3"), "/path1/path2/path3");
    }

    /**
     */
    TEST(TestFsPath, IsAbsolutePath)
    {
        ASSERT_TRUE(FsPath{"/"}.isAbsolute());
        ASSERT_FALSE(FsPath{"dir"}.isAbsolute());
        ASSERT_FALSE(FsPath{"dir/dir2"}.isAbsolute());
    }

    /**
     */
    TEST(TestFsPath, IsRelativePath)
    {
        ASSERT_FALSE(FsPath{"/"}.isRelative());
        ASSERT_FALSE(FsPath{"/dir"}.isRelative());
        ASSERT_TRUE(FsPath{"dir"}.isRelative());
        ASSERT_TRUE(FsPath{"dir/dir2"}.isRelative());
    }

    /**
     */
    TEST(TestFsPath, Append)
    {
        {
            const FsPath base = "/dir1/";
            const FsPath path = base / "dir2/" / std::string{"dir3///"};

            ASSERT_THAT(path.getString(), Eq("/dir1/dir2/dir3"));
        }

        {
            FsPath path;
            path.append("dir1");

            ASSERT_TRUE(path.isRelative());
            ASSERT_THAT(path.getString(), Eq("dir1"));
        }
    }

    /**
     */
    TEST(TestFsPath, Concat)
    {
    }

    /**
     */
    TEST(TestFsPath, GetRelativePath)
    {
        const FsPath basePath = "/dir1/dir2";
        const FsPath fullPath = "dir1/dir2/dir3/assets/myfile1.txt";
        const FsPath relativePath = fullPath.getRelativePath(basePath);

        ASSERT_FALSE(relativePath.isEmpty());
        ASSERT_TRUE(relativePath.isRelative());
        ASSERT_THAT(relativePath.getString(), Eq("dir3/assets/myfile1.txt"));
    }

    /**
     */
    TEST(TestFsPath, GetParentPath)
    {
        EXPECT_THAT(FsPath{}.getParentPath(), Eq(FsPath{}));
        EXPECT_THAT(FsPath{"/dir1/dir2/fileName.ext"}.getParentPath(), Eq(FsPath{"/dir1/dir2"}));
        EXPECT_THAT(FsPath{"/dir1/dir2"}.getParentPath(), Eq(FsPath{"/dir1"}));
        EXPECT_THAT(FsPath{"/dir1"}.getParentPath(), Eq(FsPath{"/"}));
        EXPECT_THAT(FsPath{"/"}.getParentPath(), Eq(FsPath{"/"}));
        EXPECT_THAT(FsPath{"name.txt"}.getParentPath(), Eq(FsPath{}));
        EXPECT_THAT(FsPath{"/dir1/tmp/."}.getParentPath(), Eq(FsPath{"/dir1/tmp"}));
    }

    /**
     */
    TEST(TestFsPath, GetName)
    {
        EXPECT_THAT(FsPath{"/dir1/dir2/fileName.ext"}.getName(), Eq("fileName.ext"));
        EXPECT_THAT(FsPath{"/dir1/dir2/.fileName"}.getName(), Eq(".fileName"));
        EXPECT_THAT(FsPath{"/dir1/dir2/dir3"}.getName(), Eq("dir3"));
        EXPECT_THAT(FsPath{"fileName.ext1.ext2"}.getName(), Eq("fileName.ext1.ext2"));
    }

    /**
     */
    TEST(TestFsPath, GetExtension)
    {
        EXPECT_THAT(FsPath{"/dir1/fileName.ext"}.getExtension(), Eq(".ext"));
        EXPECT_THAT(FsPath{"/dir1/fileName.ext1.ext2"}.getExtension(), Eq(".ext2"));
        EXPECT_THAT(FsPath{"/dir1/.fileName"}.getExtension(), Eq(""));
        EXPECT_THAT(FsPath{"/dir1/dir2.dir21/fileName."}.getExtension(), Eq("."));
        EXPECT_THAT(FsPath{"/dir1/dir2.dir21/fileName"}.getExtension(), Eq(""));
        EXPECT_THAT(FsPath{"/dir1/dir2.dir21/."}.getExtension(), Eq(""));
        EXPECT_THAT(FsPath{"/dir1/dir2.dir21/.."}.getExtension(), Eq(""));
        EXPECT_THAT(FsPath{"/dir1/dir2/"}.getExtension(), Eq(""));
        EXPECT_THAT(FsPath{"/dir1/..ext1"}.getExtension(), Eq(".ext1"));
    }

    /**
     */
    TEST(TestFsPath, GetStem)
    {
        EXPECT_THAT(FsPath{"/dir1/fileName.ext"}.getStem(), Eq("fileName"));
        EXPECT_THAT(FsPath{"/dir1/.fileName"}.getStem(), Eq(".fileName"));
        EXPECT_THAT(FsPath{"foo.bar.baz.tar"}.getStem(), Eq("foo.bar.baz"));
    }

    /**
     */
    TEST(TestFsPath, MakeAbsolutePath)
    {
        ASSERT_EQ(FsPath{}.makeAbsolute(), FsPath{"/"});
        ASSERT_EQ(FsPath{"/path1"}.makeAbsolute(), FsPath{"/path1"});
        ASSERT_EQ(FsPath{"path1/path2"}.makeAbsolute(), FsPath{"/path1/path2"});
        ASSERT_EQ(FsPath{"path1/path2"}.makeAbsolute(), FsPath{"/path1/path2"});
    }

    /**
     */
    TEST(TestFsPath, PathEquality)
    {
        using Hash = std::hash<FsPath>;
        Hash hash;

        ASSERT_EQ(FsPath{}, FsPath{});
        ASSERT_EQ(hash(FsPath{}), hash(FsPath{}));

        ASSERT_EQ(FsPath{"/"}, FsPath{"/"});
        ASSERT_EQ(hash(FsPath{"/"}), hash(FsPath{"/"}));

        ASSERT_EQ(FsPath{"/path1/"}, FsPath{"/path1"});
        ASSERT_EQ(hash(FsPath{"/path1/"}), hash(FsPath{"/path1"}));

        ASSERT_EQ(FsPath{"/path1/path2"}, FsPath{"/path1//path2//"});
        ASSERT_EQ(hash(FsPath{"/path1/path2"}), hash(FsPath{"/path1//path2//"}));

        ASSERT_EQ(FsPath{"/path1\\path2"}, FsPath{"/path1//path2//"});
        ASSERT_EQ(hash(FsPath{"/path1\\path2"}), hash(FsPath{"/path1//path2//"}));

        ASSERT_EQ(FsPath{"path1\\path2"}, FsPath{"path1/path2/"});
        ASSERT_EQ(hash(FsPath{"path1\\path2"}), hash(FsPath{"path1/path2/"}));
    }

    /**
     */
    TEST(TestFsPath, SplitElements)
    {
    }

    /**
     */
    TEST(TestFsPath, Hash)
    {
        std::unordered_map<FsPath, unsigned> map;
        map.emplace("/dir1", 1);
        map.emplace("/dir1/dir2", 2);
        map.emplace("/dir1/dir2/dir3", 3);

        ASSERT_THAT(map["/dir1"], Eq(1));
        ASSERT_THAT(map["/dir1/dir2"], Eq(2));
        ASSERT_THAT(map["/dir1\\dir2  "], Eq(2));
        ASSERT_THAT(map["/dir1/dir2/dir3"], Eq(3));

        {
            auto [iter, emplaceOk] = map.emplace("/dir1\\dir2\\dir3\\", 33);
            ASSERT_FALSE(emplaceOk);
        }
    }

    /**
     */
    TEST(TestFsPath, Serialization)
    {
        static_assert(StringParsable<FsPath>);

        std::u8string_view json =
            u8R"--(
            {
                "path1": "/path1/path2/",
                "path2": "  path1\\\\path2//path3"
            }
        )--";

        using Container = std::map<std::string, FsPath>;
        Result<Container> parseResult = serialization::JsonUtils::parse<Container>(json);
        ASSERT_TRUE(parseResult);
        ASSERT_EQ(parseResult->size(), 2);
        ASSERT_EQ(parseResult->at("path1"), FsPath{"/path1/path2"});
        ASSERT_EQ(parseResult->at("path2"), FsPath{"path1/path2/path3"});

        auto string2 = serialization::JsonUtils::stringify(*parseResult);
        Result<Container> parseResult2 = serialization::JsonUtils::parse<Container>(json);

        ASSERT_EQ(*parseResult, *parseResult2);
    }

}  // namespace nau::test
