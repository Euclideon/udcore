#ifndef UDVALUE_INL_H
#define UDVALUE_INL_H

inline udValue::udValue()           { Clear(); }
inline udValue::udValue(int64_t v)  { type = udVT_Int64;  u.i64Val = v; }
inline udValue::udValue(double v)   { type = udVT_Double; u.dVal   = v; }
inline void udValue::Clear()        { type = udVT_Void;   u.i64Val = 0; } // Clear the value without freeing
inline udValue::~udValue()          { Destroy(); }

// Set the value
inline void udValue::SetVoid()      { Destroy(); }
inline void udValue::Set(int64_t v) { Destroy(); type = udVT_Int64;  u.i64Val = v; }
inline void udValue::Set(double v)  { Destroy(); type = udVT_Double; u.dVal   = v; }

// Accessors
inline bool udValue::IsVoid()        const { return (type == udVT_Void); }
inline bool udValue::IsNumeric()     const { return (type >= udVT_Uint8 && type <= udVT_Double); }
inline bool udValue::IsIntegral()    const { return (type >= udVT_Uint8 && type <= udVT_Int64); }
inline bool udValue::IsString()      const { return (type == udVT_String); }
inline bool udValue::IsList()        const { return (type == udVT_List) || (type == udVT_Element); }
inline bool udValue::IsObject()      const { return (type == udVT_Object) || (type == udVT_Element); }
inline bool udValue::IsElement()     const { return (type == udVT_Element); }
inline bool udValue::HasMemory()     const { return (type >= udVT_String && type < udVT_Count); }
inline size_t udValue::ListLength()  const { udValueList *pList = AsList(); return pList ? pList->length : 0; }

inline udValueList *udValue::AsList() const               { return (type == udVT_List) ? u.pList : (type == udVT_Element) ? &u.pElement->children : nullptr; }
inline udValueObject *udValue::AsObject() const           { return (type == udVT_Object) ? u.pObject : AsElement(); }
inline udValueElement *udValue::AsElement() const         { return (type == udVT_Element) ? u.pElement : nullptr; }

#endif // UDVALUE_INL_H
