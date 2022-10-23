#ifndef BOOL_OR_ERROR_H
#define BOOL_OR_ERROR_H

enum class BoolOrError {
  False = 0,
  True = 1,
  Error = -1
};

#define CAST_BoolOrError(x) static_cast<BoolOrError>(x)

bool operator==(BoolOrError a, bool b) ;

bool operator==(bool b, BoolOrError a) ;

bool isTrue(BoolOrError b) ;


#endif /* BOOL_OR_ERROR_H */
