// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <asio.hpp>
#include <string>

#include "nau/memory/bytes_buffer.h"

namespace nau
{
    class ASIO_Connection
    {
    public:
        ASIO_Connection(asio::ip::tcp::socket s);
        ASIO_Connection(asio::io_context& io_context);

        void connect(asio::ip::tcp::endpoint endpoint, std::function<void(std::error_code)> onConnect);
        bool disconnect();
        bool isConnected() const;

        void write(const BytesBuffer& buffer);
        void read(BytesBuffer& buffer);

        const std::string& localEndPoint() const;
        const std::string& remoteEndPoint() const;

    private:
        void doWriteBuffer();
        void writeHandler(const asio::error_code& error, std::size_t bytes_transferred);

        asio::ip::tcp::socket m_socket;
        asio::streambuf m_read_buffer;
        asio::streambuf m_write_buffer;
        mutable std::string m_localEndPoint;
        mutable std::string m_remoteEndPoint;
    };

    class ASIO_Acceptor
    {
    public:
        ASIO_Acceptor(asio::io_context& io_context, short port, std::function<void(asio::ip::tcp::socket socket)> onAccept);

    private:
        void do_accept();
        void close();
        std::function<void(asio::ip::tcp::socket socket)> m_acceptCallback;

        asio::io_context& m_context;
        asio::ip::tcp::acceptor m_acceptor;
    };

}  // namespace nau
