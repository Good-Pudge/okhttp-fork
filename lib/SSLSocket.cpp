//
// Created by Good_Pudge.
//

#include "../include/SSLSocket.hpp"
#include "../include/Exception.hpp"
#include "../util/include/util.hpp"
#include <sstream>
#include <openssl/err.h>

using namespace ohf;

std::string getOpenSSLError() {
    std::string error;
    unsigned long error_code;
    while (error_code = ERR_get_error()) {
        char *str = ERR_error_string(error_code, nullptr);
        if (!str)
            return error;
        error += str;
    }
    return error;
}

SSLSocket::SSLSocket() : Socket() {
    //Init OpenSSL
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();

    ssl_context = SSL_CTX_new(SSLv23_method());
    if (!ssl_context)
        throw Exception(Exception::Code::OPENSSL_CREATE_CONTEXT_ERROR, "OpenSSL create context error:" +
                                                                       getOpenSSLError());
    ssl = SSL_new(ssl_context);
    if (!ssl) throw Exception(Exception::Code::OPENSSL_INIT_ERROR, "OpenSSL init error: " + getOpenSSLError());
}

SSLSocket::~SSLSocket() {
    SSL_CTX_free(ssl_context);
    SSL_free(ssl);
}

void SSLSocket::send(const char *data, int size) {
    int len = SSL_write(ssl, data, size);
    if (len < 0) {
        int error = SSL_get_error(ssl, len);
        if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
            return;
        throw Exception(Exception::Code::OPENSSL_ERROR, getOpenSSLError());
    }
}

std::string SSLSocket::receive(size_t size) {
    int len = 0;
    std::string storage;
    char *buffer = new char[size];
    if (size == 0) {
        size = 512;
        do {
            len = SSL_read(ssl, buffer, size);
            storage.append(buffer, len);
        } while (len > 0);
        if (len < 0) {
            int error = SSL_get_error(ssl, len);
            if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
                return storage;
            throw Exception(Exception::Code::OPENSSL_ERROR, getOpenSSLError());
        }
    } else {
        len = SSL_read(ssl, buffer, size);
        storage.append(buffer, len);
        if (len < 0) {
            int error = SSL_get_error(ssl, len);
            if (error == SSL_ERROR_WANT_WRITE || error == SSL_ERROR_WANT_READ)
                return storage;
            throw Exception(Exception::Code::OPENSSL_ERROR, getOpenSSLError());
        }
    }
    return storage;
}

#if _WIN32

std::iostream &SSLSocket::connect(const std::string &address, const int &port) {
    // Address setup
    sockaddr_in addr;
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family = AF_INET; // TCP/IP
    hostent *hosts = gethostbyname(address.c_str());
    if (hosts) { // found host
        addr.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(**(in_addr **) hosts->h_addr_list)); // Get IP from DNS
    } else throw Exception(WSAGetLastError(), "Socket error: " + util::getWSAError());
    addr.sin_port = htons(port); // Port
    // Connect
    if (::connect(s, (sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR)
        throw Exception(WSAGetLastError(), "Socket error: " + util::getWSAError());

    SSL_set_fd(ssl, s);
    if (SSL_connect(ssl) < 1)
        throw Exception(Exception::Code::FAILED_TO_CREATE_SSL_CONNECTION,
                        "Failed to create SSL connection: " + getOpenSSLError());

    // Init IO stream
    buf = std::make_shared<StreamBuf>(this);
    ios = std::make_shared<std::iostream>(buf.get());
    return *ios;
}

#endif

// ohf::SSLSocket::StreamBuf
int SSLSocket::StreamBuf::overflow(int c) {
    if (c == traits_type::eof())
        return traits_type::eof();

    char ch = (char) c;
    sock->send(&ch, 1);

    return c;
}

int SSLSocket::StreamBuf::uflow() {
    int c = underflow();
    cur = traits_type::eof();
    return c;
}

int SSLSocket::StreamBuf::underflow() {
    if (cur != traits_type::eof())
        return cur;

    std::string data = sock->receive(1);
    if (data.empty())
        return traits_type::eof();

    cur = data[0];

    return cur;
}