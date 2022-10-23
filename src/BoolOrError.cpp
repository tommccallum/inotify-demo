#include "BoolOrError.h"

bool operator==(BoolOrError a, bool b) {
  return ((a == BoolOrError::True) && b == true) || ((a== BoolOrError::False) && b == false);
}

bool operator==(bool b, BoolOrError a) {
  return ((a == BoolOrError::True) && b==true) || ((a==BoolOrError::False) && b == false);
}

bool isTrue(BoolOrError b) {
  return b == BoolOrError::True;
}
