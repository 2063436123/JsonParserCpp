//
// Created by Hello Peter on 2021/8/13.
//

#include "JsonParser.h"

#include <utility>

JElement::JType JObject::type() {
    return JType::JOBJECT;
}

std::string JObject::toJson() {
    std::string jsonStr("{");
    bool first = true;
    for (const auto &pair : objectValue_) {
        if (!first)
            jsonStr += ",";
        else
            first = false;
        jsonStr += "\"" + pair.first;
        jsonStr += "\":" + pair.second->toJson();
    }
    jsonStr += "}";
    return jsonStr;
}

std::shared_ptr<JElement> JObject::getElement(const std::string &key) {
    auto iter = objectValue_.find(key);
    if (iter == objectValue_.end())
        throw std::out_of_range("key not found.");
    return iter->second;
}

auto JObject::getMultiElements(const std::string &key) {
    auto pair_iter = objectValue_.equal_range(key);
    if (pair_iter.first == objectValue_.end())
        throw std::out_of_range("key not found.");
    return pair_iter;
}

bool JObject::hasKey(const std::string &key) const {
    return objectValue_.contains(key);
}

size_t JObject::size() const {
    return objectValue_.size();
}

auto JObject::pairs() const {
    return objectValue_;
}

void JObject::addElement(const std::string &key, std::shared_ptr<JElement> e) {
    objectValue_.insert({key, std::move(e)});
}

JElement::JType JArray::type() {
    return JType::JARRAY;
}

std::string JArray::toJson() {
    std::string jsonStr("[");
    for (size_t i = 0; i < arrayValue_.size(); i++) {
        if (i > 0)
            jsonStr += ',';
        jsonStr += arrayValue_[i]->toJson();
    }
    jsonStr += "]";
    return jsonStr;
}

size_t JArray::size() const {
    return arrayValue_.size();
}

std::shared_ptr<JElement> JArray::getElement(size_t index) const {
    return std::shared_ptr<JElement>(arrayValue_.at(index));
}

void JArray::setElement(size_t index, std::shared_ptr<JElement> e) {
    arrayValue_.at(index) = std::move(e);
}

void JArray::removeElement(size_t index) {
    arrayValue_.at(index);
    arrayValue_.erase(arrayValue_.begin() + index);
}

void JArray::addElement(std::shared_ptr<JElement> e) {
    arrayValue_.push_back(std::move(e));
}

JElement::JType JString::type() {
    return JElement::JType::JSTRING;
}

std::string JString::toJson() {
    return '"' + strValue_ + '"';
}

JString::JString(const char *s, size_t len) : strValue_(s, len) {}

// 不使用const string&而是string + move，这样可以以一个函数实现左值复制，右值移动
JString::JString(std::string str) : strValue_(std::move(str)) {}

std::string JString::getStr() const {
    return strValue_;
}

JElement::JType JTrue::type() {
    return JElement::JType::JTRUE;
}

std::string JTrue::toJson() {
    return "true";
}

JElement::JType JFalse::type() {
    return JElement::JType::JFALSE;
}

std::string JFalse::toJson() {
    return "false";
}

JElement::JType JNull::type() {
    return JElement::JType::JNULL;
}

std::string JNull::toJson() {
    return "null";
}

std::string JElement::getAsString() {
    JString* js = dynamic_cast<JString*>(this);
    if (!js)
        throw std::bad_cast();
    return js->getStr();
}

double JElement::getAsDouble() {
    JNumber* jn = dynamic_cast<JNumber*>(this);
    if (!jn)
        throw std::bad_cast();
    return jn->getDouble();
}

bool JElement::getAsBoolean() {
    if (isJFalse())
        return false;
    if (isJTrue())
        return true;
    throw std::bad_cast();
}

std::shared_ptr<JArray> JElement::getAsArray() {
    std::shared_ptr<JArray> ja = std::dynamic_pointer_cast<JArray>(shared_from_this());
    if (!ja)
        throw std::bad_cast();
    return ja;
}

std::shared_ptr<JObject> JElement::getAsObject() {
    std::shared_ptr<JObject> jo = std::dynamic_pointer_cast<JObject>(shared_from_this());
    if (!jo)
        throw std::bad_cast();
    return jo;
}

JElement::JType JNumber::type() {
    return JType::JNUMBER;
}

std::string JNumber::toJson() {
    char buf[50];
    size_t len = sprintf(buf, "%.17g", numberValue_);
    return std::string(buf, len);
}

JNumber::JNumber(double n) : numberValue_(n) {}

double JNumber::getDouble() const {
    return numberValue_;
}

