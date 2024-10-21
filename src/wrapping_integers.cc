#include "wrapping_integers.hh"
#include <iostream>
using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) {
    return Wrap32{static_cast<uint32_t>((n + zero_point.raw_value_) % (1ul << 32))};
}

//summury: 1. the wrap function is used to convert a uint64_t value to a Wrap32 object.
//        2. the wrap function takes two parameters: a uint64_t value and a Wrap32 object.
//        3. the wrap function calculates the raw_value by adding the zero_point to the n and taking the modulus of 1ul << 32.
//        4. the wrap function returns a Wrap32 object with the raw_value.

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t base = raw_value_- zero_point.raw_value_;
  if(checkpoint > base){
    uint64_t base0 = base + (checkpoint - base)/(1ul << 32)*(1ul << 32);
    uint64_t base1 = base + (checkpoint - base)/(1ul << 32)*(1ul << 32) + (1ul << 32);
    if(checkpoint - base0 < base1 - checkpoint){
      base = base0;
    }else{
      base = base1;
    }
  }
  return base;
}

//summury: 1. the unwrap function is used to convert a Wrap32 object back to its original uint64_t value.
//        2. the unwrap function takes two parameters: a Wrap32 object and a uint64_t value.
//        3. the unwrap function calculates the base value by subtracting the zero_point from the raw_value.
//        4. if the checkpoint is greater than the base value, the function calculates two possible base values: base0 and base1.
//        5. the function then compares the difference between the checkpoint and base0 with the difference between base1 and the checkpoint.
//        6. if the difference between the checkpoint and base0 is smaller, the function sets the base value to base0.
//        7. if the difference between base1 and the checkpoint is smaller, the function sets the base value to base1.
//        8. the function returns the base value.
