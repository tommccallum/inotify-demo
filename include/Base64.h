#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

// https://stackoverflow.com/questions/5288076/base64-encoding-and-decoding-with-openssl

std::string base64(const unsigned char *input, int length) ;
std::vector<unsigned char> decode64(const char *input, int length) ;

#endif /* BASE64_H */
