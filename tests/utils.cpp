#include <gtest/gtest.h>
#include "utils.hpp"

using namespace spotifar;

TEST(utils, utf8_decode)
{
    string utf8_str = "Hello, world!";
    wstring expected_wstr = L"Hello, world!";
    
    wstring result_wstr = spotifar::utils::utf8_decode(utf8_str);
    
    EXPECT_EQ(result_wstr, expected_wstr);
}

TEST(utils, strip_invalid_filename_chars)
{
    wstring filename = L"Invalid:Filename*?\\/<>|";
    wstring expected_stripped = L"Invalid_Filename_______";
    
    wstring result_stripped = spotifar::utils::strip_invalid_filename_chars(filename);
    
    EXPECT_EQ(result_stripped, expected_stripped);
}