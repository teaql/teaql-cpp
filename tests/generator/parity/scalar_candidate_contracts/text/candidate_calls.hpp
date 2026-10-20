#pragma once
#include <cstddef>
#include <stdexcept>
// Observer adapter derived from authentic candidate metadata; not a domain-library implementation.
inline constexpr const char* CANDIDATE_SQL =
    "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO platform_data VALUES(1,'Platform',1);CREATE TABLE record_data(id INTEGER PRIMARY KEY,platform INTEGER,name TEXT,choice TEXT,version INTEGER);INSERT INTO record_data VALUES(10,1,'row-0','red',1),(11,1,'row-1','blue',1),(12,1,'row-2','null',1),(13,1,'row-3','other',1),(99,1,'deleted','red',-1);";

template <class Request>
static void candidate_filter(Request& request, std::size_t index, bool negative) {
    switch (index) {
    case 0:
        if (negative) {
            request.withChoiceIsNotRed();
        } else {
            request.withChoiceIsRed();
        }
        break;
    case 1:
        if (negative) {
            request.withChoiceIsNotBlue();
        } else {
            request.withChoiceIsBlue();
        }
        break;
    case 2:
        if (negative) {
            request.withChoiceIsNotNull();
        } else {
            request.withChoiceIsNull();
        }
        break;
    case 3:
        if (negative) {
            request.withChoiceIsKnown();
        } else {
            request.withChoiceIsUnknown();
        }
        break;
    case 4:
        if (negative) {
            request.withChoiceIsUnknown();
        } else {
            request.withChoiceIsKnown();
        }
        break;
    default:
        throw std::runtime_error("Unknown candidate index");
    }
}
