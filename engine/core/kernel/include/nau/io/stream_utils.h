// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/dag_ioSys/dag_baseIo.h"
#include "nau/dag_ioSys/dag_genIo.h"
#include "nau/diag/assertion.h"
#include "nau/io/stream.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::io_detail
{
    /**
     */
    template <typename Char>
    requires(sizeof(Char) == sizeof(char))
    class StringWriterImpl final : public io::IStreamWriter
    {
        NAU_CLASS(StringWriterImpl<Char>, rtti::RCPolicy::StrictSingleThread, IStreamWriter)

    public:
        using String = std::basic_string<Char>;

        StringWriterImpl(String& outputString) :
            m_string(outputString)
        {
        }

    private:
        size_t getPosition() const override
        {
            return m_string.size();
        }

        size_t setPosition(io::OffsetOrigin, int64_t) override
        {
            NAU_ASSERT("StringWriterImpl::setOffset is not implemented");
            return 0;
        }

        Result<size_t> write(const std::byte* ptr, size_t count) override
        {
            constexpr size_t ReserveBlockSize = 16;

            if ((count < ReserveBlockSize) && (m_string.capacity() < m_string.size() + ReserveBlockSize))
            {
                m_string.reserve(m_string.size() + ReserveBlockSize);
            }

            m_string.append(reinterpret_cast<const Char*>(ptr), count);
            return count;
        }

        void flush() override
        {
        }

        String& m_string;
    };

    /**
     */
    template <typename StreamImpl, typename StreamApi>
    requires(std::is_base_of_v<StreamApi, StreamImpl>)
    class InplaceStreamHolder
    {
    public:
        const nau::Ptr<StreamApi>& getStream() const
        {
            NAU_ASSERT(m_stream);
            return (m_stream);
        }

        operator StreamApi&() const
        {
            return *getStream();
        }

    protected:
        template <typename... Args>
        void createStream(Args&&... args)
        {
            NAU_FATAL(!m_stream);
            m_stream = rtti::createInstanceInplace<StreamImpl, StreamApi>(m_inplaceMemBlock, std::forward<Args>(args)...);
        }

    protected:
        nau::Ptr<StreamApi> m_stream;

    private:
        rtti::InstanceInplaceStorage<StreamImpl> m_inplaceMemBlock;
    };

}  // namespace nau::io_detail

namespace nau::io
{
    template <typename Char = char8_t>
    class InplaceStringWriter : public io_detail::InplaceStreamHolder<io_detail::StringWriterImpl<Char>, IStreamWriter>
    {
    public:
        InplaceStringWriter(std::basic_string<Char>& outputString)
        {
            this->createStream(outputString);
        }
    };

    /**
     */
    class NAU_KERNEL_EXPORT GenLoadOverStream : public iosys::IBaseLoad
    {
    public:
        GenLoadOverStream(IStreamReader::Ptr stream, std::string_view targetName = {});
        GenLoadOverStream(const GenLoadOverStream&) = delete;
        GenLoadOverStream(GenLoadOverStream&&) = default;

        GenLoadOverStream& operator=(const GenLoadOverStream&) = delete;
        GenLoadOverStream& operator=(GenLoadOverStream&&) = default;

        void read(void* ptr, int size) override;

        int tryRead(void* ptr, int size) override;

        int tell() override;

        void seekto(int position) override;

        void seekrel(int offset) override;

        const char* getTargetName() override;

    private:
        IStreamReader::Ptr m_stream;
        std::string m_targetName;
    };

}  // namespace nau::io
