uint getLower(const uint val) { return val & uint(0xFFFF); }
uint getUpper(const uint val) { return val >> 16 & uint(0xFFFF); }
float fixed_to_float(const uint val) { return float(val)/32.0; }
vec2 fixed2_to_vec2(const uint val) { return vec2(fixed_to_float(getLower(val)), fixed_to_float(getUpper(val))); }
vec2 uint_to_vec2(const uint val) { return vec2(getLower(val), getUpper(val)); }
