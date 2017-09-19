//
// Created by Good_Pudge.
//

#ifndef OKHTTPFORK_MEDIATYPE_HPP
#define OKHTTPFORK_MEDIATYPE_HPP

#include <string>

namespace ohf {
    class MediaType {
    public:
        MediaType() = default;

        explicit MediaType(const std::string &str);

        MediaType(const char *str);

        std::string boundary();

        std::string boundary(const std::string &defaultValue);

        std::string charset();

        std::string charset(const std::string &defaultValue);

        bool operator==(const MediaType &mediaType); // TODO: Compare values

        std::string subtype();

        std::string type();

    private:
        std::string boundary_;
        std::string charset_;
        std::string subtype_;
        std::string type_;
    };
}

#endif //OKHTTPFORK_MEDIATYPE_HPP
