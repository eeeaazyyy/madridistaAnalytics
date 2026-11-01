#ifndef ijgfduifrijni8727jr2hsodfib
#define ijgfduifrijni8727jr2hsodfib

#include <string>

namespace madridista { namespace domain {

enum class ErrorCode {
    InvalidArgument,
    InvalidResponse,
    NotFound,
    Storage,
};

struct Error {
    ErrorCode code;
    std::string message;
};

}} // namespace madridista::domain

#endif