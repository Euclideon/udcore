#ifndef UDVALUE_INL_H
#define UDVALUE_INL_H

inline udValue::udValue()           { Clear(); }
inline udValue::udValue(uint8_t v)  { type = udVT_Uint8;  arrayLength = 0; u.u8Val  = v; }
inline udValue::udValue(int32_t v)  { type = udVT_Int32;  arrayLength = 0; u.i32Val = v; }
inline udValue::udValue(int64_t v)  { type = udVT_Int64;  arrayLength = 0; u.i64Val = v; }
inline udValue::udValue(float v)    { type = udVT_Float;  arrayLength = 0; u.fVal   = v; }
inline udValue::udValue(double v)   { type = udVT_Double; arrayLength = 0; u.dVal   = v; }
inline void udValue::Clear()        { type = udVT_Void;   arrayLength = 0; u.i64Val = 0; } // Clear the value without freeing
inline udValue::~udValue()          { Destroy(); }

// Set the value
inline void udValue::SetVoid()      { Destroy(); }
inline void udValue::Set(uint8_t v) { Destroy(); type = udVT_Uint8;  arrayLength = 0; u.u8Val  = v; }
inline void udValue::Set(int32_t v) { Destroy(); type = udVT_Int32;  arrayLength = 0; u.i32Val = v; }
inline void udValue::Set(int64_t v) { Destroy(); type = udVT_Int64;  arrayLength = 0; u.i64Val = v; }
inline void udValue::Set(float v)   { Destroy(); type = udVT_Float;  arrayLength = 0; u.fVal   = v; }
inline void udValue::Set(double v)  { Destroy(); type = udVT_Double; arrayLength = 0; u.dVal   = v; }

// Accessors
inline bool udValue::IsVoid()        const { return (type == udVT_Void); }
inline bool udValue::IsNumeric()     const { return (type >= udVT_Uint8 && type <= udVT_Double); }
inline bool udValue::IsIntegral()    const { return (type >= udVT_Uint8 && type <= udVT_Int64); }
inline bool udValue::IsString()      const { return (type == udVT_String); }
inline bool udValue::IsList()        const { return (type == udVT_List); }
inline bool udValue::IsObject()      const { return (type == udVT_Object); }
inline bool udValue::IsElement()     const { return (type == udVT_Element); }
inline bool udValue::HasMemory()     const { return (type >= udVT_String && type < udVT_Count); }
inline size_t udValue::ListLength()  const { udValueList *pList = AsList(); return pList ? pList->length : 0; }
inline size_t udValue::ArrayLength() const { return (size_t)arrayLength; }

// Array access
inline bool *udValue::GetArrayBool(size_t *pLength)     { if (type == udVT_Bool)    { if (arrayLength) { *pLength = arrayLength; return (bool    *)u.pArray;  } else { *pLength = 1; return &u.bVal;   } } else return nullptr; }
inline uint8_t *udValue::GetArrayUint8(size_t *pLength) { if (type == udVT_Uint8)   { if (arrayLength) { *pLength = arrayLength; return (uint8_t  *)u.pArray; } else { *pLength = 1; return &u.u8Val;  } } else return nullptr; }
inline int32_t *udValue::GetArrayInt32(size_t *pLength) { if (type == udVT_Int32)   { if (arrayLength) { *pLength = arrayLength; return (int32_t *)u.pArray;  } else { *pLength = 1; return &u.i32Val; } } else return nullptr; }
inline int64_t *udValue::GetArrayInt64(size_t *pLength) { if (type == udVT_Int64)   { if (arrayLength) { *pLength = arrayLength; return (int64_t *)u.pArray;  } else { *pLength = 1; return &u.i64Val; } } else return nullptr; }
inline float *udValue::GetArrayFloat(size_t *pLength)   { if (type == udVT_Float)   { if (arrayLength) { *pLength = arrayLength; return (float *)u.pArray;    } else { *pLength = 1; return &u.fVal;   } } else return nullptr; }
inline double *udValue::GetArrayDouble(size_t *pLength) { if (type == udVT_Double)  { if (arrayLength) { *pLength = arrayLength; return (double *)u.pArray;   } else { *pLength = 1; return &u.dVal;   } } else return nullptr; }
inline char **udValue::GetArrayString(size_t *pLength)  { if (type == udVT_String)  { if (arrayLength) { *pLength = arrayLength; return (char **)u.pArray;    } else { *pLength = 1; return &u.pStr;   } } else return nullptr; }

inline udValueList *udValue::AsList() const               { return (type == udVT_List) ? u.pList : (type == udVT_Element) ? &u.pElement->children : nullptr; }
inline udValueObject *udValue::AsObject() const           { return (type == udVT_Object) ? u.pObject : AsElement(); }
inline udValueElement *udValue::AsElement() const         { return (type == udVT_Element) ? u.pElement : nullptr; }

#endif // UDVALUE_INL_H
