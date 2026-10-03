// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <string_view>

#include "nau/diag/error.h"

namespace nau::test
{
    struct NAU_ABSTRACT_TYPE ITestError : Error
    {
        NAU_ABSTRACT_ERROR(ITestError, Error)

        virtual unsigned getErrorCode() const = 0;
    };

    class TestError final : public DefaultError<ITestError>
    {
        using ErrorBase = DefaultError<ITestError>;

        NAU_ERROR(TestError, ErrorBase)

    public:
        TestError(diag::SourceInfo sourceInfo, unsigned errorCode) :
            ErrorBase(sourceInfo, "errorCode"),
            m_errorCode(errorCode)
        {
        }

        unsigned getErrorCode() const override
        {
            return m_errorCode;
        }

    private:
        const unsigned m_errorCode;
    };

    TEST(TestError, MakeDefaultError)
    {
        const char* ErrorText = "test error";

        auto error = NauMakeError(ErrorText);
        ASSERT_EQ(std::string{ErrorText}, error->getMessage());
        ASSERT_TRUE(error->is<nau::Error>());
    }

    TEST(TestError, MakeCustomError)
    {
        const std::string ErrorText = "test error";

        auto error2 = NauMakeErrorT(TestError)(100);
        ASSERT_EQ("errorCode", error2->getMessage());
        ASSERT_TRUE(error2->is<nau::Error>());
        ASSERT_TRUE(error2->is<TestError>());
    }

    TEST(TestError, ErrorIsStdException)
    {
        const std::string ErrorText = "test error";
        auto error = NauMakeError(ErrorText);

        ASSERT_TRUE(error->is<std::exception>());

        auto& exception = error->as<const std::exception&>();
        ASSERT_EQ(ErrorText, exception.what());
    }

    TEST(TestError, FormattedMessage)
    {
        auto error = NauMakeError("Text[{}][{}]", 77, 22);
        ASSERT_EQ(error->getMessage(), std::string_view{"Text[77][22]"});
    }

}  // namespace nau::test
