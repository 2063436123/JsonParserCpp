//
// Created by Hello Peter on 2021/8/15.
//
#include <cmath>
#include <cassert>
#include "JsonParser.h"

std::shared_ptr<JElement> JsonParser::parse(const char *s, size_t len) {
    return parse(std::string(s, len));
}

class JsonParserImpl {
    void skipWhite() {
        while (*p_ == ' ' || *p_ == '\t' || *p_ == '\n' || *p_ == '\r')
            ++p_;
    }

    std::shared_ptr<JTrue> parseJTrue() {
        if (*p_ != 't' || *(p_ + 1) != 'r' || *(p_ + 2) != 'u' || *(p_ + 3) != 'e')
            throw ParseError(ParseError::INVALID_VALUE, this);
        p_ += 4;
        return JTrue::New();
    }

    std::shared_ptr<JFalse> parseJFalse() {
        if (*p_ != 'f' || *(p_ + 1) != 'a' || *(p_ + 2) != 'l' || *(p_ + 3) != 's' || *(p_ + 4) != 'e')
            throw ParseError(ParseError::INVALID_VALUE, this);
        p_ += 5;
        return JFalse::New();
    }

    std::shared_ptr<JNull> parseJNull() {
        if (*p_ != 'n' || *(p_ + 1) != 'u' || *(p_ + 2) != 'l' || *(p_ + 3) != 'l')
            throw ParseError(ParseError::INVALID_VALUE, this);
        p_ += 4;
        return JNull::New();
    }

    inline bool ISDIGIT1TO9(char ch) { return '1' <= ch && ch <= '9'; }

    inline bool ISDIGIT(char ch) { return '0' <= ch && ch <= '9'; }

    std::shared_ptr<JNumber> parseJNumber() {
        const char *p = p_;
        if (*p == '-') p++;
        if (*p == '0') p++;
        else {
            if (!ISDIGIT1TO9(*p))
                throw ParseError(ParseError::INVALID_VALUE, this);
            for (p++; ISDIGIT(*p); p++);
        }
        if (*p == '.') {
            p++;
            if (!ISDIGIT(*p))
                throw ParseError(ParseError::INVALID_VALUE, this);
            for (p++; ISDIGIT(*p); p++);
        }
        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '+' || *p == '-') p++;
            if (!ISDIGIT(*p))
                throw ParseError(ParseError::INVALID_VALUE, this);
            for (p++; ISDIGIT(*p); p++);
        }
        errno = 0;
        auto n = strtod(p_, nullptr);
        if (errno == ERANGE && (n == HUGE_VAL || n == -HUGE_VAL))
            throw ParseError(ParseError::NUMBER_TOO_BIG, this);
        p_ = p;
        return JNumber::New(n);
    }

    const char *lept_parse_hex4(const char *p, unsigned *u) {
        int i;
        *u = 0;
        for (i = 0; i < 4; i++) {
            char ch = *p++;
            *u <<= 4;
            if (ch >= '0' && ch <= '9') *u |= ch - '0';
            else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
            else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
            else return nullptr;
        }
        return p;
    }

    void lept_encode_utf8(unsigned u) {
        if (u <= 0x7F)
            buffer_.push_back(u & 0xFF);
        else if (u <= 0x7FF) {
            buffer_.push_back(0xC0 | ((u >> 6) & 0xFF));
            buffer_.push_back(0x80 | (u & 0x3F));
        } else if (u <= 0xFFFF) {
            buffer_.push_back(0xE0 | ((u >> 12) & 0xFF));
            buffer_.push_back(0x80 | ((u >> 6) & 0x3F));
            buffer_.push_back(0x80 | (u & 0x3F));
        } else {
            buffer_.push_back(0xF0 | ((u >> 18) & 0xFF));
            buffer_.push_back(0x80 | ((u >> 12) & 0x3F));
            buffer_.push_back(0x80 | ((u >> 6) & 0x3F));
            buffer_.push_back(0x80 | (u & 0x3F));
        }
    }

    std::shared_ptr<JString> parseJString() {
        size_t old_buffer_size = buffer_.size();
        const char *p = p_;
        ++p;
        unsigned u, u2;
        for (;;) {
            char ch = *p++;
            switch (ch) {
                case '\"':
                    p_ = p;
                    break;
                case '\\':
                    switch (*p++) {
                        case '\"':
                            buffer_.push_back('\"');
                            break;
                        case '\\':
                            buffer_.push_back('\\');
                            break;
                        case '/':
                            buffer_.push_back('/');
                            break;
                        case 'b':
                            buffer_.push_back('\b');
                            break;
                        case 'f':
                            buffer_.push_back('\f');
                            break;
                        case 'n':
                            buffer_.push_back('\n');
                            break;
                        case 'r':
                            buffer_.push_back('\r');
                            break;
                        case 't':
                            buffer_.push_back('\t');
                            break;
                        case 'u':
                            if (!(p = lept_parse_hex4(p, &u)))
                                throw ParseError(ParseError::INVALID_UNICODE_CHAR, this);
                            if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                                if (*p++ != '\\')
                                    throw ParseError(ParseError::INVALID_UNICODE_CHAR, this);
                                if (*p++ != 'u')
                                    throw ParseError(ParseError::INVALID_UNICODE_CHAR, this);
                                if (!(p = lept_parse_hex4(p, &u2)))
                                    throw ParseError(ParseError::INVALID_UNICODE_CHAR, this);
                                if (u2 < 0xDC00 || u2 > 0xDFFF)
                                    throw ParseError(ParseError::INVALID_UNICODE_CHAR, this);
                                u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                            }
                            lept_encode_utf8(u);
                            break;
                        default:
                            throw ParseError(ParseError::INVALID_STRING_CHAR, this);
                    }
                    break;
                case '\0':
                    throw ParseError(ParseError::MISS_STRING_END_ESCAPE, this);
                default:
                    if ((unsigned char) ch < 0x20)
                        throw ParseError(ParseError::INVALID_STRING_CHAR, this);
                    buffer_.push_back(ch);
            }
            if (ch == '\"')
                break;
        }

        auto ret = JString::New(std::string(buffer_.data() + old_buffer_size, buffer_.size() - old_buffer_size));
        buffer_.erase(buffer_.begin() + old_buffer_size, buffer_.end());
        assert(buffer_.size() == old_buffer_size);
        return ret;
    }

    std::shared_ptr<JObject> parseJObject() {
        auto ret = JObject::New();
        ++p_;
        skipWhite();
        if (*p_ == '}') {
            ++p_;
            return ret;
        }
        for (;;) {
            if (*p_ != '"')
                throw ParseError(ParseError::MISS_KEY, this);
            auto key = parseJString()->getAsString();
            skipWhite();
            if (*p_ != ':')
                throw ParseError(ParseError::MISS_COLON, this);
            ++p_;
            skipWhite();

            auto sub_element = parseSingle();

            ret->addElement(key, sub_element);
            skipWhite();

            if (*p_ == ',') {
                ++p_;
                skipWhite();
                if (*p_ == '\0')
                    throw ParseError(ParseError::REDUNDANT_COMMA, this);
            } else if (*p_ == '}') {
                ++p_;
                break;
            } else {
                throw ParseError(ParseError::MISS_COMMA_OR_CURLY_BRACKET, this);
            }
        }
        return ret;
    }

    std::shared_ptr<JArray> parseJArray() {
        auto ret = JArray::New();
        ++p_;
        skipWhite();
        if (*p_ == ']') {
            ++p_;
            return ret;
        }
        for (;;) {
            auto sub_element = parseSingle();
            ret->addElement(sub_element);
            skipWhite();
            if (*p_ == ',') {
                ++p_;
                skipWhite();
                if (*p_ == '\0')
                    throw ParseError(ParseError::REDUNDANT_COMMA, this);
            } else if (*p_ == ']') {
                ++p_;
                break;
            } else {
                throw ParseError(ParseError::MISS_COMMA_OR_SQUARE_BRACKET, this);
            }
        }
        return ret;
    }

    // 只为parseJArray/JObject服务，不进行成员变量的存储和skipWhite操作.
    std::shared_ptr<JElement> parseSingle() {
        std::shared_ptr<JElement> ret;
        switch (*p_) {
            case 't':
                ret = parseJTrue();
                break;
            case 'f':
                ret = parseJFalse();
                break;
            case 'n':
                ret = parseJNull();
                break;
            case '"':
                ret = parseJString();
                break;
            case '{':
                ret = parseJObject();
                break;
            case '[':
                ret = parseJArray();
                break;
            default:
                ret = parseJNumber();
                break;
        }
        return ret;
    }

public:
    JsonParserImpl() {
        buffer_.reserve(50);
    }

    std::shared_ptr<JElement> parse(const std::string &str) {
        std::shared_ptr<JElement> ret;
        str_ = str;
        p_ = str_.data();
        skipWhite();
        switch (*p_) {
            case 't':
                ret = parseJTrue();
                break;
            case 'f':
                ret = parseJFalse();
                break;
            case 'n':
                ret = parseJNull();
                break;
            case '"':
                ret = parseJString();
                break;
            case '{':
                ret = parseJObject();
                break;
            case '[':
                ret = parseJArray();
                break;
            default:
                ret = parseJNumber();
                break;
        }
        skipWhite();
        if (*p_ != '\0')
            throw ParseError(ParseError::REDUNDANT_CHARS, this);
        return ret;
    }

private:
    friend class ParseError;

    const char *p_; // 指向当前的处理位置，in [str_.begin(), str_.end()]
    std::string str_; // 保存str副本，供ParseError使用
    std::vector<char> buffer_; // 缓冲区，供parseJString使用
};

ParseError::ParseError(Error e, JsonParserImpl *impl) {
    switch (e) {
        case INVALID_VALUE:
            msg_ = "invalid value";
            break;
        case NUMBER_TOO_BIG:
            msg_ = "number too big";
            break;
        case REDUNDANT_CHARS:
            msg_ = "redundant chars";
            break;
        case INVALID_STRING_CHAR:
            msg_ = "invalid string char";
            break;
        case INVALID_UNICODE_CHAR:
            msg_ = "invalid unicode char";
            break;
        case MISS_STRING_END_ESCAPE:
            msg_ = "miss string end escape";
            break;
        case MISS_COMMA_OR_SQUARE_BRACKET:
            msg_ = "miss comma or square bracket";
            break;
        case MISS_KEY:
            msg_ = "miss key";
            break;
        case MISS_COLON:
            msg_ = "miss colon";
            break;
        case MISS_COMMA_OR_CURLY_BRACKET:
            msg_ = "miss comma or curly bracket";
            break;
        case REDUNDANT_COMMA:
            msg_ = "redundant comma";
            break;
        default:
            msg_ = "wtf?";
            break;
    }
    msg_ += ". which near:\n";
    msg_ += impl->str_ + "\n";
    msg_ += std::string(impl->p_ - impl->str_.data(), ' ') + "^\n";
}

JsonParser::JsonParser() : impl_(std::make_unique<JsonParserImpl>()) {}

JsonParser::~JsonParser() = default;

std::shared_ptr<JElement> JsonParser::parse(const std::string &str) {
    return impl_->parse(str);
}

