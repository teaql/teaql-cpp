#pragma once
#include <cstddef>
#include <stdexcept>
// Observer adapter derived from authentic candidate metadata; not a domain-library implementation.
inline constexpr const char* CANDIDATE_SQL =
    "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);INSERT INTO platform_data VALUES(1,'Platform',1);CREATE TABLE record_data(id INTEGER PRIMARY KEY,platform INTEGER,name TEXT,choice NUMERIC,version INTEGER);INSERT INTO record_data VALUES(10,1,'row-0',1.5,1),(11,1,'row-1',2.5,1),(12,1,'row-2',9.5,1),(99,1,'deleted',1.5,-1);";

template <class Request>
static void candidate_filter(Request& request, std::size_t index, bool negative) {
    switch (index) {
    case 0:
        if (negative) {
            request.withChoiceIsNotValue15();
        } else {
            request.withChoiceIsValue15();
        }
        break;
    case 1:
        if (negative) {
            request.withChoiceIsNotValue25();
        } else {
            request.withChoiceIsValue25();
        }
        break;
    case 2:
        if (negative) {
            request.withChoiceIsKnown();
        } else {
            request.withChoiceIsUnknown();
        }
        break;
    case 3:
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
