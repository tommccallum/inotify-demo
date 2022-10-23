#include "Base64.h"
#include <openssl/evp.h>
#include <iostream>

// https://stackoverflow.com/questions/5288076/base64-encoding-and-decoding-with-openssl

std::string base64(const unsigned char *input, int length) {
  const auto pl = 4*((length+2)/3);
  std::string output(pl+1, '\0');
  const auto ol = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(output.data()), input, length);
  if (pl != ol) { std::cerr << "Whoops, encode predicted " << pl << " but we got " << ol << "\n"; }
  return output;
}

std::vector<unsigned char> decode64(const char *input, int length) {
  const auto pl = 3*length/4;
  std::vector<unsigned char> output(pl+1, '\0');
  const auto ol = EVP_DecodeBlock(output.data(), reinterpret_cast<const unsigned char *>(input), length);
  if (pl != ol) { std::cerr << "Whoops, decode predicted " << pl << " but we got " << ol << "\n"; }
  return output;
}
