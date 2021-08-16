#include <gtest/gtest.h>
#include "JsonParser.h"
#include <iostream>


TEST(Renderer, BaseTypes) {
    JNull jn;
    EXPECT_TRUE(jn.isJNull());
    EXPECT_EQ(jn.toJson(), "null");
    JTrue jt;
    EXPECT_TRUE(jt.isJTrue());
    EXPECT_EQ(jt.toJson(), "true");
    JFalse jf;
    EXPECT_TRUE(jf.isJFalse());
    EXPECT_EQ(jf.toJson(), "false");
    JString js("hello", 5);
    EXPECT_TRUE(js.isJString());
    EXPECT_EQ(js.toJson(), "\"hello\"");
    JNumber jnu(123.55);
    EXPECT_TRUE(jnu.isNumber());
    EXPECT_EQ(jnu.toJson(), "123.55");
}

TEST(Renderer, ObjectType) {
    JObject jo;
    EXPECT_TRUE(jo.isJObject());
    EXPECT_EQ(jo.toJson(), "{}");
    std::shared_ptr<JArray> sp = std::make_shared<JArray>();
    sp->addElement(std::make_shared<JString>("baskte"));
    sp->addElement(std::make_shared<JString>("tennis"));
    jo.addElement("hobby", sp);
    jo.addElement("sex", std::make_shared<JString>("female"));
    EXPECT_EQ(jo.toJson(), "{\"sex\":\"female\",\"hobby\":[\"baskte\",\"tennis\"]}");
    EXPECT_TRUE(jo.hasKey("sex"));
    EXPECT_FALSE(jo.hasKey("what"));
    jo.getElement("hobby")->getAsArray()->setElement(0, std::make_shared<JString>("basket"));
    EXPECT_EQ(jo.getElement("hobby")->getAsArray()->getElement(0)->getAsString(), "basket");
    EXPECT_EQ(jo.getElement("sex")->getAsString(), "female");
    EXPECT_EQ(jo.size(), 2);
}

TEST(Renderer, ArrayType) {
    JArray ja;
    EXPECT_TRUE(ja.isJArray());
    EXPECT_EQ(ja.toJson(), "[]");
    ja.addElement(std::make_shared<JFalse>());
    ja.addElement(std::make_shared<JNull>());
    ja.addElement(std::make_shared<JString>("hello world"));
    EXPECT_EQ(ja.toJson(), "[false,null,\"hello world\"]");
    EXPECT_EQ(ja.size(), 3);
    EXPECT_TRUE(ja.getElement(0)->isJFalse());
    EXPECT_TRUE(ja.getElement(1)->isJNull());
    EXPECT_EQ(ja.getElement(2)->toJson(), "\"hello world\"");
    ja.removeElement(0);
    ja.removeElement(0);
    EXPECT_EQ(ja.size(), 1);
    ja.setElement(0, std::make_shared<JString>("fucku"));
    EXPECT_EQ(ja.getElement(0)->getAsString(), "fucku");
    EXPECT_EQ(ja.toJson(), "[\"fucku\"]");
    EXPECT_THROW(ja.getElement(1), std::out_of_range);
    EXPECT_THROW(ja.setElement(1, std::make_shared<JString>("wo")), std::out_of_range);
    EXPECT_THROW(ja.removeElement(1), std::out_of_range);
}

TEST(Parser, BaseTypes) {
    JsonParser parser;
    auto ret = parser.parse("null");
    EXPECT_TRUE(ret->isJNull());
    ret = parser.parse("false");
    EXPECT_TRUE(ret->isJFalse());
    ret = parser.parse("true");
    EXPECT_TRUE(ret->isJTrue());
}

TEST(Parser, StringType) {
    JsonParser parser;
    EXPECT_EQ(parser.parse("\"hello world\"")->getAsString(), "hello world");
    EXPECT_EQ(parser.parse("\"Hello\\nWorld\"")->getAsString(), "Hello\nWorld");
    EXPECT_EQ(parser.parse("\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"")->getAsString(), "\" \\ / \b \f \n \r \t");
    EXPECT_EQ(parser.parse("\"Hello\\u0000World\"")->getAsString(), std::string("Hello\0World", 11));
    EXPECT_EQ(parser.parse("\"\\u0024\"")->getAsString(), "\x24");
    EXPECT_EQ(parser.parse("\"\\u00A2\"")->getAsString(), "\xC2\xA2");
    EXPECT_EQ(parser.parse("\"\\u20AC\"")->getAsString(), "\xE2\x82\xAC");
    EXPECT_EQ(parser.parse("\"\\uD834\\uDD1E\"")->getAsString(), "\xF0\x9D\x84\x9E");
    EXPECT_EQ(parser.parse("\"\\ud834\\udd1e\"")->getAsString(), "\xF0\x9D\x84\x9E");

    EXPECT_THROW(parser.parse("\"\\v\""), ParseError);
    EXPECT_THROW(parser.parse("\"\x01\""), ParseError);
    EXPECT_THROW(parser.parse("\"\\u0G00\""), ParseError);
}

TEST(Parser, NumberType) {
    JsonParser parser;
    EXPECT_EQ(parser.parse("0")->getAsDouble(), 0.0);
    EXPECT_EQ(parser.parse("-0")->getAsDouble(), 0.0);
    EXPECT_EQ(parser.parse("-0.0")->getAsDouble(), 0.0);
    EXPECT_EQ(parser.parse("1")->getAsDouble(), 1.0);
    EXPECT_EQ(parser.parse("-1")->getAsDouble(), -1.0);
    EXPECT_EQ(parser.parse("1.5")->getAsDouble(), 1.5);
    EXPECT_EQ(parser.parse("-1.5")->getAsDouble(), -1.5);
    EXPECT_EQ(parser.parse("3.1416")->getAsDouble(), 3.1416);
    EXPECT_EQ(parser.parse("1E10")->getAsDouble(), 1E10);
    EXPECT_EQ(parser.parse("1e10")->getAsDouble(), 1e10);
    EXPECT_EQ(parser.parse("1E+10")->getAsDouble(), 1E+10);
    EXPECT_EQ(parser.parse("1e-10")->getAsDouble(), 1e-10);
    EXPECT_EQ(parser.parse("1.234E+10")->getAsDouble(), 1.234E+10);
    EXPECT_EQ(parser.parse("1e-10000")->getAsDouble(), 0.0);

    EXPECT_EQ(parser.parse("1.0000000000000002")->getAsDouble(), 1.0000000000000002);
    EXPECT_EQ(parser.parse("4.9406564584124654e-324")->getAsDouble(), 4.9406564584124654e-324);
    EXPECT_EQ(parser.parse("2.2250738585072009e-308")->getAsDouble(), 2.2250738585072009e-308);
    EXPECT_EQ(parser.parse("2.2250738585072014e-308")->getAsDouble(), 2.2250738585072014e-308);
    EXPECT_EQ(parser.parse("1.7976931348623157e+308")->getAsDouble(), 1.7976931348623157e+308);
    EXPECT_THROW(parser.parse("1e309"), ParseError);
    EXPECT_THROW(parser.parse("-1e309"), ParseError);
}

TEST(Parser, ArrayType) {
    JsonParser parser;
    auto arr1 = parser.parse("[ null , false , true , 123 , \"abc\" ]")->getAsArray();
    EXPECT_EQ(arr1->size(), 5);
    EXPECT_TRUE(arr1->getElement(0)->isJNull());
    EXPECT_TRUE(arr1->getElement(1)->isJFalse());
    EXPECT_TRUE(arr1->getElement(2)->isJTrue());
    EXPECT_EQ(arr1->getElement(3)->getAsDouble(), 123);
    EXPECT_EQ(arr1->getElement(4)->getAsString(), "abc");
}

TEST(Parser, ObjectType) {
    JsonParser parser;
    auto jo = parser.parse(" { "
                 "\"n\" : null , "
                 "\"f\" : false , "
                 "\"t\" : true , "
                 "\"i\" : 123 , "
                 "\"s\" : \"abc\", "
                 "\"a\" : [ 1, 2, 3 ],"
                 "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                 " } ")->getAsObject();
    EXPECT_EQ(jo->size(), 7);
    EXPECT_TRUE(jo->getElement("n")->isJNull());
    EXPECT_TRUE(jo->getElement("f")->isJFalse());
    EXPECT_TRUE(jo->getElement("t")->isJTrue());
    EXPECT_EQ(jo->getElement("i")->getAsDouble(), 123);
    EXPECT_EQ(jo->getElement("s")->getAsString(), "abc");
    EXPECT_EQ(jo->getElement("a")->getAsArray()->size(), 3);
    EXPECT_EQ(jo->getElement("o")->getAsObject()->getElement("2")->getAsDouble(), 2.0);
}

TEST(Parser, ParseError) {
    JsonParser parser;
    try {
//    parser.parse("[ \"hello\"], ");
        auto jo = parser.parse("{\n"
                     "    \"status\": \"0000\",\n"
                     "    \"message\": \"success\",\n"
                     "    \"data\": {\n"
                     "        \"title\": {\n"
                     "            \"id\": \"001\",\n"
                     "            \"name\" : \"白菜\"\n"
                     "        },\n"
                     "        \"content\": [\n"
                     "            {\n"
                     "                \"id\": \"001\",\n"
                     "                \"value\":\"你好 白菜\"\n"
                     "            },\n"
                     "            {\n"
                     "                \"id\": \"002\",\n"
                     "                 \"value\":\"你好 萝卜\" \n"
                     "            }\n"
                     "        ]\n"
                     "    }\n"
                     "}")->getAsObject();
        EXPECT_EQ(jo->size(), 3);
        EXPECT_EQ(jo->getElement("status")->getAsString(), "0000");
        std::cout << "message: " <<  jo->getElement("message") << std::endl;
        auto data = jo->getElement("data")->getAsObject();
        std::cout << "data.title: " << data->getElement("title")->getAsObject()->getElement("id")->getAsString() << ", " << data->getElement("title")->getAsObject()->getElement("name")->getAsString() << std::endl;
        std::cout << "data.content" << data->getElement("content")->toJson() << std::endl;

    } catch (ParseError& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
