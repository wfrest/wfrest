#include <gtest/gtest.h>
#include <map>
#include <unordered_map>
#include "wfrest/StringPiece.h"

using namespace wfrest;

TEST(StringPiece, constructor)
{
    std::string str = "0123456";
    int cursor = 1;
    StringPiece strp1(&str[cursor], 4);
    EXPECT_EQ("1234", strp1.as_string());

    StringPiece strp2(strp1.data() + cursor, 2);
    EXPECT_EQ("23", strp2.as_string());
}

TEST(StringPiece, map)
{
    std::map<StringPiece, int> piece_map;
    std::string a = "12345";
    StringPiece b(a.c_str() + 1, 3);

    EXPECT_EQ("234", b.as_string());
    piece_map[b] = 1;   // "234" : 1

    StringPiece c("234");
    auto it = piece_map.find(c);
    EXPECT_TRUE(it != piece_map.end());
    EXPECT_EQ(piece_map[c], 1);

    char *addr = &*a.begin()+1;
    EXPECT_EQ(addr, b.data());
    EXPECT_EQ(addr, it->first.data());
}

void proc_param(StringPiece& str)
{
    int i = 1;
    int j = str.size() - 2;
    while(str[i] == ' ') i++;
    while(str[j] == ' ') j--;
    str.shrink(i, str.size() - 1 - j);
}

TEST(StringPiece, get_parameter)
{
    StringPiece str1("<    name   >");
    proc_param(str1);
    EXPECT_EQ("name", str1.as_string());

    StringPiece str2("<name>");
    proc_param(str2);
    EXPECT_EQ("name", str2.as_string());

    StringPiece str3("<>");
    proc_param(str3);
    EXPECT_EQ("", str3.as_string());
}

TEST(StringPiece, wildcast)
{
    StringPiece origin("action*");
    StringPiece match(origin);
    match.remove_suffix(1);
    StringPiece str1("action1");
    StringPiece str2("action");
    StringPiece str3("action123123");
    EXPECT_TRUE(str1.starts_with(match));
    EXPECT_TRUE(str2.starts_with(match));
    EXPECT_TRUE(str3.starts_with(match));
}

TEST(StringPiece, shrink)
{
    StringPiece str("1234567890");
    str.shrink(0, 2);
    EXPECT_EQ("12345678", str.as_string());
}

TEST(StringPiece, normalizes_null_and_default_views)
{
    StringPiece empty;
    EXPECT_NE(empty.data(), nullptr);
    EXPECT_EQ(empty.begin(), empty.end());
    EXPECT_TRUE(empty.empty());
    EXPECT_TRUE(empty.as_string().empty());

    std::string copied = "not-empty";
    empty.CopyToString(&copied);
    EXPECT_TRUE(copied.empty());

    const StringPiece literal_empty("");
    EXPECT_EQ(empty, literal_empty);
    EXPECT_EQ(empty.compare(literal_empty), 0);
    EXPECT_TRUE(empty.starts_with(literal_empty));
    EXPECT_EQ(StringPieceHash()(empty), StringPieceHash()(literal_empty));

    size_t iterations = 0;
    for (char value : empty)
    {
        static_cast<void>(value);
        ++iterations;
    }
    EXPECT_EQ(iterations, 0U);

    const StringPiece null_string(static_cast<const char *>(nullptr));
    const StringPiece null_bytes(static_cast<const char *>(nullptr), 10);
    const StringPiece null_void(static_cast<const void *>(nullptr), 10);
    EXPECT_EQ(null_string, empty);
    EXPECT_EQ(null_bytes, empty);
    EXPECT_EQ(null_void, empty);
    EXPECT_NE(null_string.data(), nullptr);
    EXPECT_NE(null_bytes.data(), nullptr);
    EXPECT_NE(null_void.data(), nullptr);

    empty.clear();
    EXPECT_NE(empty.data(), nullptr);
    EXPECT_TRUE(empty.empty());
}

TEST(StringPiece, normalizes_setter_lengths)
{
    const char buffer[] = "abc";
    StringPiece piece("value");

    piece.set(static_cast<const char *>(nullptr));
    EXPECT_NE(piece.data(), nullptr);
    EXPECT_TRUE(piece.empty());

    piece.set(static_cast<const char *>(nullptr), static_cast<size_t>(3));
    EXPECT_NE(piece.data(), nullptr);
    EXPECT_TRUE(piece.empty());

    piece.set(static_cast<const void *>(nullptr), 3U);
    EXPECT_NE(piece.data(), nullptr);
    EXPECT_TRUE(piece.empty());

    piece.set(buffer, -1);
    EXPECT_NE(piece.data(), nullptr);
    EXPECT_TRUE(piece.empty());

    piece.set(buffer, 0);
    EXPECT_EQ(piece.data(), buffer);
    EXPECT_TRUE(piece.empty());

    piece.set(buffer, 2);
    EXPECT_EQ(piece.data(), buffer);
    EXPECT_EQ(piece.as_string(), "ab");
}

TEST(StringPiece, saturates_removal_at_current_size)
{
    const char input[] = "abc";

    StringPiece prefix(input);
    prefix.remove_prefix(99);
    EXPECT_TRUE(prefix.empty());
    EXPECT_EQ(prefix.data(), input + 3);
    prefix.remove_prefix(1);
    EXPECT_EQ(prefix.data(), input + 3);
    EXPECT_TRUE(prefix.as_string().empty());

    StringPiece suffix(input);
    suffix.remove_suffix(99);
    EXPECT_TRUE(suffix.empty());
    EXPECT_EQ(suffix.data(), input);
    suffix.remove_suffix(1);
    EXPECT_EQ(suffix.data(), input);

    StringPiece both(input);
    both.shrink(2, 99);
    EXPECT_TRUE(both.empty());
    EXPECT_EQ(both.data(), input + 2);

    StringPiece oversized_prefix(input);
    oversized_prefix.shrink(99, 99);
    EXPECT_TRUE(oversized_prefix.empty());
    EXPECT_EQ(oversized_prefix.data(), input + 3);
}
