#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"
//
// Created by Hello Peter on 2021/8/13.
//

#ifndef JSONPARSER_JSONPARSER_H
#define JSONPARSER_JSONPARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>

/** 头文件只提供用户直接访问的接口 */

class JsonParserImpl;

class ParseError : public std::exception {
public:
    enum Error {
        INVALID_VALUE,
        NUMBER_TOO_BIG,
        REDUNDANT_CHARS,
        INVALID_STRING_CHAR,
        INVALID_UNICODE_CHAR,
        MISS_STRING_END_ESCAPE,
        MISS_COMMA_OR_SQUARE_BRACKET,
        MISS_KEY,
        MISS_COLON,
        MISS_COMMA_OR_CURLY_BRACKET,
        REDUNDANT_COMMA
    };

    ParseError(Error e, JsonParserImpl *impl);

    const char *what() const noexcept override {
        return msg_.data();
    }

private:
    std::string msg_;
};

// 前置声明，供getAsArray/Object使用.
class JArray;

class JObject;

class JElement : public std::enable_shared_from_this<JElement> {
public:
    enum class JType {
        JNULL, JTRUE, JFALSE, JOBJECT, JARRAY, JSTRING, JNUMBER
    };

    virtual JType type() = 0;

    virtual std::string toJson() = 0;

    bool isJNull() {
        return type() == JType::JNULL;
    }

    bool isJTrue() {
        return type() == JType::JTRUE;
    }

    bool isJFalse() {
        return type() == JType::JFALSE;
    }

    bool isJObject() {
        return type() == JType::JOBJECT;
    }

    bool isJArray() {
        return type() == JType::JARRAY;
    }

    bool isJString() {
        return type() == JType::JSTRING;
    }

    bool isNumber() {
        return type() == JType::JNUMBER;
    }

    /* 便利函数，内部执行了向下转型操作 */
    std::string getAsString();

    double getAsDouble();

    bool getAsBoolean();

    std::shared_ptr<JArray> getAsArray();

    std::shared_ptr<JObject> getAsObject();
};

class JNull : public JElement {
public:
    static std::shared_ptr<JNull> New() {
        return std::make_shared<JNull>();
    }

    JType type() override;

    std::string toJson() override;
};

class JTrue : public JElement {
public:
    static std::shared_ptr<JTrue> New() {
        return std::make_shared<JTrue>();
    }

    JType type() override;

    std::string toJson() override;
};

class JFalse : public JElement {
public:
    static std::shared_ptr<JFalse> New() {
        return std::make_shared<JFalse>();
    }

    JType type() override;

    std::string toJson() override;
};

class JObject : public JElement {
public:
    static std::shared_ptr<JObject> New() {
        return std::make_shared<JObject>();
    }

    JType type() override;

    std::string toJson() override;

    size_t size() const;

    auto pairs() const;

    bool hasKey(const std::string &key) const;

    std::shared_ptr<JElement> getElement(const std::string &key);

    auto getMultiElements(const std::string &key);

    void addElement(const std::string &key, std::shared_ptr<JElement> e);

private:
    std::unordered_multimap<std::string, std::shared_ptr<JElement>> objectValue_;
};

class JArray : public JElement {
public:
    static std::shared_ptr<JArray> New() {
        return std::make_shared<JArray>();
    }

    JType type() override;

    std::string toJson() override;

    size_t size() const;

    std::shared_ptr<JElement> getElement(size_t index) const;

    void setElement(size_t index, std::shared_ptr<JElement> e);

    void addElement(std::shared_ptr<JElement> e);

    void removeElement(size_t index);

private:
    /* 因多态需要，使用shared_ptr类型 */
    std::vector<std::shared_ptr<JElement>> arrayValue_;
};

class JString : public JElement {
public:
    static std::shared_ptr<JString> New(const char *s, size_t len) {
        return New(std::string(s, len));
    }

    static std::shared_ptr<JString> New(std::string str) {
        return std::make_shared<JString>(std::move(str));
    }

    JType type() override;

    std::string toJson() override;

    JString(const char *s, size_t len);

    explicit JString(std::string str);

    std::string getStr() const;

private:
    std::string strValue_;
};

class JNumber : public JElement {
public:
    static std::shared_ptr<JNumber> New(double n) {
        return std::make_shared<JNumber>(n);
    }

    JType type() override;

    std::string toJson() override;

    explicit JNumber(double n);

    double getDouble() const;

private:
    double numberValue_;
};


class JsonParser {
public:
    JsonParser();

    std::shared_ptr<JElement> parse(const char *s, size_t len);

    std::shared_ptr<JElement> parse(const std::string &str);

    ~JsonParser();

private:
    std::unique_ptr<JsonParserImpl> impl_;
};


#endif //JSONPARSER_JSONPARSER_H

#pragma clang diagnostic pop