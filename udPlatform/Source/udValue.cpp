#include "udValue.h"
#include "udPlatform.h"
#include "udCrypto.h"

const udValue udValue::s_void;
const size_t udValue::s_udValueTypeSize[udVT_Count] =
{
  0, // udVT_Void = 0,      // Guaranteed to be zero, thus a non-zero type indicates value exists
  1, // udVT_Bool,
  1, // udVT_Uint8,
  4, // udVT_Int32,
  8, // udVT_Int64,
  4, // udVT_Float,
  8, // udVT_Double,
  sizeof(char*), // udVT_String,
  sizeof(udValueList*), // udVT_List,
  sizeof(udValueObject*), // udVT_Object,
};

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
// Very small expression parsing helper
class udJSONExpression
{
public:
  char op, nextOp;
  const char *pKey;
  char *pRemainingExpression;
  bool isExpression;

  void Init(char *pKeyExpression)
  {
    pRemainingExpression = const_cast<char*>(udStrSkipWhiteSpace(pKeyExpression));
    nextOp = 0;
    if (*pRemainingExpression == '[' || *pRemainingExpression == '.')
    {
      nextOp = *pRemainingExpression;
      pRemainingExpression = const_cast<char*>(udStrSkipWhiteSpace(pRemainingExpression + 1));
    }
    Next();
  }
  void InitKeyOnly(const char *pKeyExpression)
  {
    nextOp = op = 0;
    pKey = pKeyExpression; // We know in this case we won't modify
    pRemainingExpression = nullptr;
  }
  void Next()
  {
    op = nextOp;
    char *pTemp = pRemainingExpression;
    if (pTemp)
    {
      pRemainingExpression = const_cast<char*>(udStrchr(pTemp, ".["));
      if (pRemainingExpression)
      {
        nextOp = *pRemainingExpression;
        *pRemainingExpression++ = 0;
      }
      // Trim trailing space
      size_t i = udStrlen(pTemp);
      while (i > 0 && (pTemp[i - 1] == ' ' || pTemp[i - 1] == '\t' || pTemp[i - 1] == '\r' || pTemp[i - 1] == '\n'))
        pTemp[--i] = 0;
    }
    // Skip leading space and assign to pKey
    pKey = udStrSkipWhiteSpace(pTemp); // udStrSkipWhiteSpace handles nulls gracefully
  }
};


// ****************************************************************************
// Author: Dave Pevreal, April 2017
void udValue::Destroy()
{
  if (type == udVT_String)
  {
    udFree(u.pStr);
  }
  else if (type == udVT_Object)
  {
    for (size_t i = 0; i < u.pObject->attributes.length; ++i)
    {
      udValueObject::KVPair *pItem = u.pObject->attributes.GetElement(i);
      udFree(pItem->pKey);
      pItem->value.Destroy();
    }
    u.pObject->attributes.Deinit();
    udFree(u.pObject);
  }
  else if (type == udVT_Element)
  {
    for (size_t i = 0; i < u.pElement->attributes.length; ++i)
    {
      udValueObject::KVPair *pItem = u.pElement->attributes.GetElement(i);
      if (pItem)
      {
        udFree(pItem->pKey);
        pItem->value.Destroy();
      }
    }
    u.pElement->attributes.Deinit();
    for (size_t i = 0; i < u.pElement->children.length; ++i)
    {
      udValue *pChild = u.pElement->children.GetElement(i);
      if (pChild)
        pChild->Destroy();
    }
    u.pElement->children.Deinit();
    udFree(u.pElement->pName);
    udFree(u.pElement);
  }
  else if (type == udVT_List)
  {
    for (size_t i = 0; i < u.pList->length; ++i)
    {
      udValue *pChild = u.pList->GetElement(i);
      if (pChild)
        pChild->Destroy();
    }
    u.pList->Deinit();
    udFree(u.pList);
  }

  type = udVT_Void;
  u.i64Val = 0;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetString(const char *pStr)
{
  Destroy();
  u.pStr = udStrdup(pStr);
  if (u.pStr)
  {
    type = udVT_String;
    return udR_Success;
  }
  return udR_MemoryAllocationFailure;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetList()
{
  udResult result;
  udValueList *pTempList = nullptr;
  Destroy();

  pTempList = udAllocType(udValueList, 1, udAF_Zero);
  UD_ERROR_NULL(pTempList, udR_MemoryAllocationFailure);
  result = pTempList->Init();
  UD_ERROR_HANDLE();

  type = udVT_List;
  u.pList = pTempList;
  pTempList = nullptr;
  result = udR_Success;

epilogue:
  udFree(pTempList);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::SetObject()
{
  udResult result;
  udValueObject *pTempObject = nullptr;
  Destroy();

  pTempObject = udAllocType(udValueObject, 1, udAF_Zero);
  UD_ERROR_NULL(pTempObject, udR_MemoryAllocationFailure);
  result = pTempObject->attributes.Init();
  UD_ERROR_HANDLE();

  type = udVT_Object;
  u.pObject = pTempObject;
  pTempObject = nullptr;
  result = udR_Success;

epilogue:
  udFree(pTempObject);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::SetElement()
{
  udResult result;
  udValueElement *pTempElement = nullptr;
  Destroy();

  pTempElement = udAllocType(udValueElement, 1, udAF_Zero);
  UD_ERROR_NULL(pTempElement, udR_MemoryAllocationFailure);
  result = pTempElement->attributes.Init();
  UD_ERROR_HANDLE();
  result = pTempElement->children.Init();
  UD_ERROR_HANDLE();

  type = udVT_Element;
  u.pElement = pTempElement;
  pTempElement = nullptr;
  result = udR_Success;

epilogue:
  udFree(pTempElement);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble3 &v)
{
  udResult result;
  Destroy();
  result = SetList();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pList->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble4 &v)
{
  udResult result;
  Destroy();
  result = SetList();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pList->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udQuaternion<double> &v)
{
  udResult result;
  Destroy();
  result = SetList();
  UD_ERROR_HANDLE();
  for (size_t i = 0; i < v.ElementCount; ++i)
    u.pList->PushBack(udValue(v[i]));

epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::Set(const udDouble4x4 &v, bool shrink)
{
  udResult result = udR_Success;
  size_t elements = 16;
  if (type != udVT_List || u.pList->length != 0)
  {
    Destroy();
    result = SetList();
    UD_ERROR_HANDLE();
  }
  if (shrink)
  {
    if ((v.axis.x.w == 0.0) && (v.axis.y.w == 0.0) && (v.axis.z.w == 0.0) && (v.axis.t.w == 1.0))
    {
      elements = 12;
      if ((v.axis.t.x == 0.0) && (v.axis.t.y == 0.0) && (v.axis.t.z == 0.0))
        elements = 9;
    }
  }
  switch (elements)
  {
    case 9:
    case 12:
      for (size_t i = 0; i < elements; ++i)
      {
        result = u.pList->PushBack(udValue(v.a[(i / 3) * 4 + (i % 3)]));
        UD_ERROR_HANDLE();
      }
      break;
    default:
      for (size_t i = 0; i < elements; ++i)
      {
        result = u.pList->PushBack(udValue(v.a[i]));
        UD_ERROR_HANDLE();
      }
      break;
  }
epilogue:
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
bool udValue::IsEqualTo(const udValue &other) const
{
  // Simple case of actual complete binary equality
  if (type == other.type && u.i64Val == other.u.i64Val)
    return true;
  if (IsNumeric() && other.IsNumeric())
    return AsDouble() == other.AsDouble();
  if (IsString())
    return udStrEqual(AsString(), other.AsString());
  // TODO: Consider whether we want to add test for List, Object and Element types
  return false;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
bool udValue::AsBool(bool defaultValue) const
{
  switch (type)
  {
    case udVT_Bool:    return u.bVal;
    case udVT_Int64:   return u.i64Val != 0;
    case udVT_Double:  return u.dVal != 0.0;
    case udVT_String:  return udStrEquali(u.pStr, "true");
    case udVT_Element: return (u.pElement->children.length == 1) ? u.pElement->children[0].AsBool(defaultValue) : defaultValue;
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
int udValue::AsInt(int32_t defaultValue) const
{
  switch (type)
  {
    case udVT_Bool:    return (int)u.bVal;
    case udVT_Int64:   return (int)u.i64Val;
    case udVT_Double:  return (int)u.dVal;
    case udVT_String:  return      udStrAtoi(u.pStr);
    case udVT_Element: return (u.pElement->children.length == 1) ? u.pElement->children[0].AsInt(defaultValue) : defaultValue;
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
int64_t udValue::AsInt64(int64_t defaultValue) const
{
  switch (type)
  {
    case udVT_Bool:    return (int64_t)u.bVal;
    case udVT_Int64:   return          u.i64Val;
    case udVT_Double:  return (int64_t)u.dVal;
    case udVT_String:  return          udStrAtoi64(u.pStr);
    case udVT_Element: return (u.pElement->children.length == 1) ? u.pElement->children[0].AsInt64(defaultValue) : defaultValue;
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
double udValue::AsDouble(double defaultValue) const
{
  switch (type)
  {
    case udVT_Bool:    return (double)u.bVal;
    case udVT_Int64:   return (double)u.i64Val;
    case udVT_Double:  return         u.dVal;
    case udVT_String:  return         udStrAtof64(u.pStr);
    case udVT_Element: return (u.pElement->children.length == 1) ? u.pElement->children[0].AsDouble(defaultValue) : defaultValue;
    default:
      return defaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
const char *udValue::AsString(char *pDefaultValue) const
{
  switch (type)
  {
    case udVT_String:  return u.pStr;
    case udVT_Element: return (u.pElement->children.length == 1) ? u.pElement->children[0].AsString(pDefaultValue) : pDefaultValue;
    default:
      return pDefaultValue;
  }
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble3 udValue::AsDouble3() const
{
  udDouble3 ret;
  if (type == udVT_List && u.pList->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pList->GetElement(i)->AsDouble();
  }
  return ret.zero();
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble4 udValue::AsDouble4() const
{
  udDouble4 ret;
  if (type == udVT_List && u.pList->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pList->GetElement(i)->AsDouble();
  }
  return ret.zero();
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udQuaternion<double> udValue::AsQuaternion() const
{
  udQuaternion<double> ret = udQuaternion<double>::identity();
  if (type == udVT_List && u.pList->length >= ret.ElementCount)
  {
    for (size_t i = 0; i < ret.ElementCount; ++i)
      ret[i] = u.pList->GetElement(i)->AsDouble();
  }
  return ret.zero();
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udDouble4x4 udValue::AsDouble4x4() const
{
  udDouble4x4 ret = udDouble4x4::identity();
  switch (ListLength())
  {
    case 9:
    case 12:
      for (size_t i = 0; i < u.pList->length; ++i)
        ret.a[(i / 3) * 4 + (i % 3)] = u.pList->GetElement(i)->AsDouble();
      return ret;
    case 16:
      for (size_t i = 0; i < u.pList->length; ++i)
        ret.a[i] = u.pList->GetElement(i)->AsDouble();
      return ret;
    default:
      return ret.identity();
  }
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
static size_t FindMatch(const udValueList *pList, const udValueObject *pSearchObj, int resultNumber, size_t i = 0)
{
  size_t attributesMatched = 0;
  for (; i < pList->length; ++i) // For each element in the list
  {
    attributesMatched = 0;
    const udValueObject *pCurrent = const_cast<udValue*>(pList->GetElement(i))->AsObject();
    if (!pCurrent)
      continue;
    size_t j;
    for (j = 0; (attributesMatched < pSearchObj->attributes.length) && (j < pCurrent->attributes.length); ++j) // For each attribute in each element
    {
      size_t k;
      for (k = 0; k < pSearchObj->attributes.length; ++k) // For each attribute in the search expression
      {
        if (udStrEqual(pSearchObj->attributes[k].pKey, pCurrent->attributes[j].pKey)
          && pSearchObj->attributes[k].value.IsEqualTo(pCurrent->attributes[j].value))
        {
          ++attributesMatched;
          break;
        }
      }
    }
    if ((attributesMatched == pSearchObj->attributes.length) && resultNumber-- == 0)
    {
      // All attributes in the search object were matched, we have our element
      break;
    }
  }
  return i;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
static udResult udJSON_GetVA(const udValue *pRoot, udValue **ppValue, const char *pKeyExpression, va_list ap)
{
  udResult result;
  char *pDup = nullptr; // Allocated string for sprintf'd expression
  udJSONExpression exp;

  UD_ERROR_NULL(pRoot, udR_InvalidParameter_);
  UD_ERROR_NULL(pKeyExpression, udR_InvalidParameter_);
  if (udStrchr(pKeyExpression, "%.["))
  {
    va_list apTemp;
    va_copy(apTemp, ap);
    size_t expressionLength = udSprintfVA(nullptr, 0, pKeyExpression, apTemp);
    va_end(apTemp);
    pDup = udAllocType(char, expressionLength + 1, udAF_None);
    UD_ERROR_NULL(pDup, udR_MemoryAllocationFailure);
    udSprintfVA(pDup, expressionLength + 1, pKeyExpression, ap);
    exp.Init(pDup);
  }
  else
  {
    exp.InitKeyOnly(pKeyExpression);
  }

  for (; exp.pKey; exp.Next())
  {
    switch (exp.op)
    {
      case '[':
        {
          int charCount;
          const udValueList *pList = pRoot->AsList();
          UD_ERROR_NULL(pList, udR_ObjectTypeMismatch);

          if (exp.pKey[0] == '{')
          {
            // A JSON expression within square brackets is a search for one or more key/value pair matches
            udValue searchExp;
            int resultNumber = 0;
            result = searchExp.Parse(exp.pKey, &charCount);
            UD_ERROR_HANDLE();
            if (exp.pKey[charCount] == ',')
            {
              int tempCharCount;
              resultNumber = udStrAtoi(exp.pKey + charCount + 1, &tempCharCount);
              charCount += tempCharCount + 1;
            }
            UD_ERROR_IF(charCount == 0 || exp.pKey[charCount] != ']', udR_ParseError);
            UD_ERROR_IF(!searchExp.IsObject(), udR_ParseError);
            size_t i = FindMatch(pList, searchExp.AsObject(), resultNumber);
            pRoot = (i < pList->length) ? pList->GetElement(i) : nullptr;
          }
          else
          {
            int index = udStrAtoi(exp.pKey, &charCount);
            UD_ERROR_IF(charCount == 0 || exp.pKey[charCount] != ']', udR_ParseError);
            pRoot = pList->GetElement(index < 0 ? pList->length + index : index);
          }
          UD_ERROR_NULL(pRoot, udR_ObjectNotFound);
        }
        break;
      case '.':
        // Fall through to default case
      default: // op == 0 here
        {
          udValueObject *pObject = pRoot->AsObject();
          // Check that the type is a KVStore allowing a dot dereference to be valid
          UD_ERROR_NULL(pObject, udR_ObjectTypeMismatch);
          size_t i;
          for (i = 0; i < pObject->attributes.length; ++i)
          {
            const udValueObject::KVPair *pItem = pObject->attributes.GetElement(i);
            if (udStrEqual(pItem->pKey, exp.pKey))
            {
              pRoot = &pItem->value;
              break;
            }
          }
          UD_ERROR_IF(i == pObject->attributes.length, udR_ObjectNotFound);
        }
        break;
    }
  }

  if (ppValue)
    *ppValue = const_cast<udValue*>(pRoot);
  result = udR_Success;

epilogue:
  udFree(pDup);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Get(udValue **ppValue, const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_GetVA(this, ppValue, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
const udValue &udValue::Get(const char * pKeyExpression, ...) const
{
  udValue *pValue = nullptr;
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_GetVA(this, &pValue, pKeyExpression, ap);
  va_end(ap);
  return (result == udR_Success) ? *pValue : udValue::s_void;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
static udResult udJSON_SetVA(udValue *pRoot, udValue *pSetToValue, const char *pKeyExpression, va_list ap)
{
  udResult result;
  char *pDup = nullptr; // Allocated string for sprintf'd expression
  udJSONExpression exp;
  udValue setToValueFromOperator;

  UD_ERROR_NULL(pRoot, udR_InvalidParameter_);
  UD_ERROR_NULL(pKeyExpression, udR_InvalidParameter_);
  if (!pRoot->IsObject())
  {
    result = pRoot->SetObject();
    UD_ERROR_HANDLE();
  }

  if (udStrchr(pKeyExpression, "%.[="))
  {
    va_list apTemp;
    va_copy(apTemp, ap);
    size_t expressionLength = udSprintfVA(nullptr, 0, pKeyExpression, apTemp);
    va_end(apTemp);
    pDup = udAllocType(char, expressionLength + 1, udAF_None);
    UD_ERROR_NULL(pDup, udR_MemoryAllocationFailure);
    udSprintfVA(pDup, expressionLength + 1, pKeyExpression, ap);
    // Parse the assignment result here before initialising exp
    char *pEquals = const_cast<char*>(udStrchr(pDup, "="));
    if (pEquals)
    {
      UD_ERROR_IF(pSetToValue != nullptr, udR_InvalidConfiguration);
      *pEquals++ = 0;
      if (*pEquals)
      {
        result = setToValueFromOperator.Parse(pEquals);
        UD_ERROR_HANDLE();
        pSetToValue = &setToValueFromOperator;
      }
    }
    exp.Init(pDup);
  }
  else
  {
    exp.InitKeyOnly(pKeyExpression);
  }

  for (; exp.pKey; exp.Next())
  {
    switch (exp.op)
    {
      case '[':
      {
        int charCount;
        int index = udStrAtoi(exp.pKey, &charCount);
        UD_ERROR_IF(exp.pKey[charCount] != ']', udR_ParseError);
        if (!pRoot->IsList())
        {
          result = pRoot->SetList();
          UD_ERROR_HANDLE();
        }
        udValueList *pList = pRoot->AsList();
        if (charCount == 0)
        {
          // An empty array access (ie mykey[]) indicates wanting to add to the list
          result = pList->PushBack(&pRoot);
          pRoot->Clear();
        }
        else
        {
          // Negative indices refer to end-relative existing keys (eg -1 is last element)
          if (index < 0)
            index = (int)(pList->length + index);
          UD_ERROR_IF(index < 0, udR_ObjectNotFound);
          if (pSetToValue)
          {
            // For the moment, error if assign out of bounds
            UD_ERROR_IF((size_t)index > pList->length, udR_CountExceeded);
            // If a specific index is being assigned and there's less elements, pad with voids
            while (pList->length <= (size_t)index)
            {
              result = pList->PushBack(udValue::s_void);
              UD_ERROR_HANDLE();
            }
          }
          if (!pSetToValue && !exp.pRemainingExpression)
          {
            // Reached the end of the expression, so this item needs to be removed
            if (index < pList->length)
              pList->RemoveAt(index);
            pRoot = nullptr;
          }
          else
          {
            pRoot = pList->GetElement(index);
            UD_ERROR_NULL(pRoot, udR_InternalError); // This should never happen
          }
        }
      }
      break;
      case '.':
        if (pRoot->IsVoid() && pSetToValue)
        {
          // A void key can be converted into an object automatically
          result = pRoot->SetObject();
          UD_ERROR_HANDLE();
        }
        // Check that the type is an object allowing a dot dereference to be valid
        UD_ERROR_IF(!pRoot->IsObject(), udR_ObjectTypeMismatch);
        // Fall through to default case
      default: // op == 0 here
      {
        udValueObject::KVPair *pItem;
        udValueObject *pObject = pRoot->AsObject();
        UD_ERROR_NULL(pObject, udR_ObjectNotFound);
        size_t i;
        for (i = 0; i < pObject->attributes.length; ++i)
        {
          pItem = const_cast<udValueObject::KVPair *>(pObject->attributes.GetElement(i));
          if (udStrEqual(pItem->pKey, exp.pKey))
          {
            if (!pSetToValue && !exp.pRemainingExpression)
            {
              // Reached the end of the expression, so this item needs to be removed
              udFree(pItem->pKey);
              pItem->value.Destroy();
              pObject->attributes.RemoveAt(i);
              pRoot = nullptr;
            }
            else
            {
              pRoot = &pItem->value;
            }
            break;
          }
        }
        if (pSetToValue && i == pObject->attributes.length)
        {
          UD_ERROR_IF(!pSetToValue, udR_ObjectNotFound);
          result = pObject->attributes.PushBack(&pItem);
          UD_ERROR_HANDLE();
          pItem->pKey = udStrdup(exp.pKey);
          UD_ERROR_NULL(pItem->pKey, udR_MemoryAllocationFailure);
          pRoot = &pItem->value;
          pItem->value.Clear(); // Init to void, will be assigned as required
        }
      }
      break;
    }
  }
  if (pSetToValue)
  {
    UD_ERROR_NULL(pRoot, udR_InternalError);
    *pRoot = *pSetToValue;
    pSetToValue->Clear(); // Clear it out without destroying
  }
  result = udR_Success;

epilogue:
  udFree(pDup);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Set(const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_SetVA(this, nullptr, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Set(udValue *pValue, const char *pKeyExpression, ...)
{
  va_list ap;
  va_start(ap, pKeyExpression);
  udResult result = udJSON_SetVA(this, pValue, pKeyExpression, ap);
  va_end(ap);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Parse(const char *pString, int *pCharCount, int *pLineNumber)
{
  udResult result;
  int totalCharCount = 0;
  UD_ERROR_NULL(pString, udR_InvalidParameter_);
  if (pLineNumber)
    *pLineNumber = 1;

  pString = udStrSkipWhiteSpace(pString, nullptr, &totalCharCount);
  Destroy();
  if (*pString == '{' || *pString == '[')
  {
    int charCount;
    result = ParseJSON(pString, &charCount, pLineNumber);
    UD_ERROR_HANDLE();
    totalCharCount += charCount;
  }
  else if (*pString == '<')
  {
    int charCount;
    result = ParseXML(pString, &charCount, pLineNumber);
    UD_ERROR_HANDLE();
    totalCharCount += charCount;
  }
  else if (*pString == '\"' || *pString == '\'') // Allow single quotes
  {
    size_t endPos = udStrMatchBrace(pString);
    // Force a parse error if the string isn't quoted properly
    UD_ERROR_IF(pString[endPos - 1] != pString[0], udR_ParseError);
    char *pStr = udAllocType(char, endPos, udAF_None);
    UD_ERROR_NULL(pStr, udR_MemoryAllocationFailure);
    size_t di = 0;
    for (size_t si = 1; si < (endPos - 1); ++si)
    {
      if (pString[si] == '\\')
      {
        switch (pString[++si])
        {
          case 'a': pStr[di++] = '\a'; break;
          case 'b': pStr[di++] = '\b'; break;
          case 'e': pStr[di++] =   27; break; // GCC extension for escape character
          case 'f': pStr[di++] = '\f'; break;
          case 'n': pStr[di++] = '\n'; break;
          case 'r': pStr[di++] = '\r'; break;
          case 't': pStr[di++] = '\t'; break;
          case 'v': pStr[di++] = '\v'; break;
          default:
            // By default, anything else following the \ is literal, allowing \" \\ \/ etc
            pStr[di++] = pString[si];
        }
      }
      else
      {
        pStr[di++] = pString[si];
      }
      UDASSERT(di < endPos, "string length miscalculation");
    }
    pStr[di] = 0;
    type = udVT_String;
    u.pStr = pStr;
    totalCharCount += (int)endPos;
  }
  else
  {
    int charCount = 0;
    int64_t i = udStrAtoi64(pString, &charCount);
    if (charCount)
    {
      if (pString[charCount] == '.')
      {
        u.dVal = udStrAtof64(pString, &charCount);
        type = udVT_Double;
      }
      else
      {
        u.i64Val = i;
        type = udVT_Int64;
      }
    }
    else if (udStrBeginsWithi(pString, "true"))
    {
      charCount = 4;
      u.bVal = true;
      type = udVT_Bool;
    }
    else if (udStrBeginsWithi(pString, "false"))
    {
      charCount = 5;
      u.bVal = false;
      type = udVT_Bool;
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }
    totalCharCount += charCount;
  }
  result = udR_Success;

epilogue:
  if (pCharCount)
    *pCharCount = totalCharCount;
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
udResult udValue::ExportValue(const char *pKey, udValue::LineList *pLines, int indent, ExportValueOptions options, bool comma) const
{
  udResult result = udR_Success;
  const char *pStr = nullptr;
  static char pEmpty[] = "";
  const char *pComma = (comma) ? "," : pEmpty;
  const char *pKeyText = nullptr;
  if (pKey && (options & EVO_XML))
    result = udSprintf(&pKeyText, "%s=", pKey);
  else if (pKey)
    result = udSprintf(&pKeyText, "\"%s\":%s", pKey, (options & EVO_StripWhiteSpace) ? "" : " ");
  else
    pKeyText = pEmpty;
  if (options & EVO_StripWhiteSpace)
    indent = 0;

  switch (type)
  {
    case udVT_Void:    result = udSprintf(&pStr, "%*s%snull%s", indent, "", pKeyText, pComma); break;
    case udVT_Bool:    result = udSprintf(&pStr, "%*s%s%s%s", indent, "", pKeyText, u.bVal ? "true" : "false", pComma); break;
    case udVT_Int64:   result = udSprintf(&pStr, "%*s%s%lld%s", indent, "", pKeyText, u.i64Val, pComma); break;
    case udVT_Double:  result = udSprintf(&pStr, "%*s%s%lf%s", indent, "", pKeyText, u.dVal, pComma); break;
    case udVT_String:  result = udSprintf(&pStr, "%*s%s\"%s\"%s", indent, "", pKeyText, u.pStr, pComma); break;
    case udVT_List:
    {
      if (!(options & EVO_XML))
      {
        result = udSprintf(&pStr, "%*s%s[", indent, "", pKeyText);
        UD_ERROR_HANDLE();
        result = pLines->PushBack(pStr);
        UD_ERROR_HANDLE();
        pStr = nullptr;
      }
      udValueList *pList = AsList();
      for (size_t i = 0; i < pList->length; ++i)
      {
        result = pList->GetElement(i)->ExportValue(nullptr, pLines, indent + 2, options, i < (pList->length - 1));
        UD_ERROR_HANDLE();
      }
      if (!(options & EVO_XML))
      {
        result = udSprintf(&pStr, "%*s]%s", indent, "", pComma);
      }
    }
    break;

    case udVT_Object:
    {
      result = udSprintf(&pStr, "%*s%s{", indent, "", pKeyText);
      UD_ERROR_HANDLE();
      result = pLines->PushBack(pStr);
      UD_ERROR_HANDLE();
      pStr = nullptr;
      udValueObject *pObject = AsObject();
      for (size_t i = 0; i < pObject->attributes.length; ++i)
      {
        udValueObject::KVPair *pItem = pObject->attributes.GetElement(i);
        result = pItem->value.ExportValue(pItem->pKey, pLines, indent + 2, options, i < (pObject->attributes.length - 1));
        UD_ERROR_HANDLE();
      }
      result = udSprintf(&pStr, "%*s}%s", indent, "", pComma);
    }
    break;

    case udVT_Element:
    {
      bool tagClosed = false;
      udValueElement *pElement = AsElement();
      const char *pTempStr;
      result = udSprintf(&pStr, "%*s%s<%s%s", indent, "", pKeyText, pElement->pName, pElement->attributes.length ? "" : ">");
      UD_ERROR_HANDLE();
      // Export all the attributes to a separate list to be combined to a single line
      LineList attributeLines;
      attributeLines.Init();
      for (size_t i = 0; i < pElement->attributes.length; ++i)
      {
        const udValueObject::KVPair &attribute = pElement->attributes[i];
        UD_ERROR_IF(attribute.value.type < udVT_Bool || attribute.value.type > udVT_String, udR_ObjectTypeMismatch);
        result = attribute.value.ExportValue(attribute.pKey, &attributeLines, 0, EVO_XML, false);
        UD_ERROR_HANDLE();
        const char *pAttrText;
        if (attributeLines.PopBack(&pAttrText))
        {
          // Combine the element onto the tag line, appending a closing or self-closing tag as required
          const char *pClose = "";
          if (i == (pElement->attributes.length - 1))
          {
            if (pElement->children.length)
            {
              pClose = ">";
            }
            else
            {
              pClose = "/>";
              tagClosed = true;
            }
          }

          result = udSprintf(&pTempStr, "%s %s%s", pStr, pAttrText,  pClose);
          udFree(pAttrText);
          UD_ERROR_HANDLE();
          udFree(pStr);
          pStr = pTempStr;
        }
      }
      // If a single string value, combine to one line
      if (pElement->children.length == 1 && pElement->children[0].type == udVT_String)
      {
        // Combine the value and closing tag
        result = udSprintf(&pTempStr, "%s%s</%s>", pStr, pElement->children[0].AsString(), pElement->pName);
        UD_ERROR_HANDLE();
        udFree(pStr);
        pStr = pTempStr;
        tagClosed = true;
      }

      attributeLines.Deinit();
      result = pLines->PushBack(pStr);
      UD_ERROR_HANDLE();
      pStr = nullptr;

      if (!tagClosed)
      {
        for (size_t i = 0; i < pElement->children.length; ++i)
          pElement->children[i].ExportValue(nullptr, pLines, indent + 2, EVO_XML, false);
        if (pElement->attributes.length == 0 || pElement->children.length > 0)
        {
          result = udSprintf(&pStr, "%*s%s</%s>", indent, "", pKeyText, pElement->pName);
          UD_ERROR_HANDLE();
        }
      }
    }
    break;

    default:
      UD_ERROR_SET(udR_InternalError);
  }
  UD_ERROR_HANDLE();
  if (pStr)
  {
    result = pLines->PushBack(pStr);
    UD_ERROR_HANDLE();
    pStr = nullptr;
  }
  result = udR_Success;

epilogue:
  udFree(pStr);
  if (pKeyText != pEmpty)
    udFree(pKeyText);
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, April 2017
udResult udValue::Export(const char **ppText, udValue::ExportOption option) const
{
  udResult result;
  udValue::LineList lines;
  size_t totalChars;
  char *pText = nullptr;

  lines.Init();
  UD_ERROR_NULL(ppText, udR_InvalidParameter_);

  result = ExportValue(nullptr, &lines, 0, (ExportValueOptions)option, false);
  UD_ERROR_HANDLE();

  totalChars = 0;
  for (size_t i = 0; i < lines.length; ++i)
    totalChars += udStrlen(lines[i]) + ((option & EO_StripWhiteSpace) ? 0 : 2);
  pText = udAllocType(char, totalChars + 1, udAF_Zero);
  totalChars = 0;
  for (size_t i = 0; i < lines.length; ++i)
  {
    size_t len = udStrlen(lines[i]);
    memcpy(pText + totalChars, lines[i], len);
    totalChars += len;
    if (!(option & EO_StripWhiteSpace))
    {
      memcpy(pText + totalChars, "\r\n", 2);
      totalChars += 2;
    }
  }
  *ppText = pText;
  pText = nullptr;

epilogue:
  for (size_t i = 0; i < lines.length; ++i)
    udFree(lines[i]);
  lines.Deinit();
  return result;
}

// ****************************************************************************
// Author: Dave Pevreal, May 2017
udResult udValue::CalculateHMAC(uint8_t hmac[32], size_t hmacLen, const uint8_t *pKey, size_t keyLen) const
{
  udResult result;
  const char *pExport = nullptr;

  result = Export(&pExport, EO_StripWhiteSpace);
  UD_ERROR_HANDLE();
  result = udCrypto_HMAC(udCH_SHA256, pKey, keyLen, (const uint8_t*)pExport, udStrlen(pExport), hmac, hmacLen);

epilogue:
  udFree(pExport);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, April 2017
udResult udValue::ParseJSON(const char *pJSON, int *pCharCount, int *pLineNumber)
{
  udResult result = udR_Success;
  const char *pStartPointer = pJSON; // Just used to calculate and assign pCharCount
  int charCount;

  pJSON = udStrSkipWhiteSpace(pJSON, nullptr, pLineNumber);
  if (*pJSON == '{')
  {
    // Handle an embedded JSON object
    result = SetObject();
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    while (*pJSON != '}')
    {
      udValueObject::KVPair *pItem;
      result = AsObject()->attributes.PushBack(&pItem);
      UD_ERROR_HANDLE();
      pItem->pKey = nullptr;
      pItem->value.Clear();

      udValue k; // Temporaries
      UD_ERROR_IF(*pJSON != '"' && *pJSON != '\'', udR_ParseError);
      result = k.Parse(pJSON, &charCount); // Use parser to get the key string for convenience
      UD_ERROR_HANDLE();
      UD_ERROR_IF(!k.IsString(), udR_ParseError);
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
      // Now add the string, taking ownership of the memory from the k temporary
      pItem->pKey = k.AsString();
      k.Clear(); // Clear k without freeing the memory, as it belongs to pItem now

      // Check and move past the colon following the key
      UD_ERROR_IF(*pJSON != ':', udR_ParseError);
      pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);

      // Parse the type, it could be an object, array, or simple type
      result = pItem->value.ParseJSON(pJSON, &charCount, pLineNumber);
      UD_ERROR_HANDLE();
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);

      if (*pJSON != '}' && *pJSON != ',')
        udDebugPrintf("JSON Parse warning: line %d, an extraneous comma found at end of object\n", *pLineNumber);
      if (*pJSON == ',')
        pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    }
    ++pJSON; // Skip the final close brace

  }
  else if (*pJSON == '[')
  {
    // Handle an array of values
    result = SetList();
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    while (*pJSON != ']')
    {
      udValue *pNextItem;
      result = AsList()->PushBack(&pNextItem);
      UD_ERROR_HANDLE();
      pNextItem->Clear();
      result = pNextItem->ParseJSON(pJSON, &charCount, pLineNumber);
      UD_ERROR_HANDLE();
      pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
      if (*pJSON != ']' && *pJSON != ',')
        udDebugPrintf("JSON Parse warning: line %d, an extraneous comma found at end of array\n", *pLineNumber);
      if (*pJSON == ',')
        pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber);
    }
    pJSON = udStrSkipWhiteSpace(pJSON + 1, nullptr, pLineNumber); // Skip the closing square bracket
  }
  else
  {
    // Case where the JSON is actually just a value
    result = Parse(pJSON, &charCount);
    if (result == udR_ParseError)
      udDebugPrintf("Error parsing JSON text, line %d: ...%.30s...\n", *pLineNumber, pJSON);
    UD_ERROR_HANDLE();
    pJSON = udStrSkipWhiteSpace(pJSON + charCount, nullptr, pLineNumber);
    UD_ERROR_SET(udR_Success);
  }

epilogue:
  if (pCharCount)
    *pCharCount = int(pJSON - pStartPointer);
  return result;
}

// ----------------------------------------------------------------------------
// Author: Dave Pevreal, May 2017
udResult udValue::ParseXML(const char *pXML, int *pCharCount, int *pLineNumber)
{
  udResult result = udR_Success;
  const char *pStartPointer = pXML; // Just used to calculate and assign pCharCount
  int charCount;
  size_t len;
  udValueElement *pElement = nullptr;
  bool selfClosingTag;

  pXML = udStrSkipWhiteSpace(pXML, nullptr, pLineNumber);
  // Just skip the xml version tag, this is ignored and not retained
  if (udStrBeginsWith(pXML, "<?xml"))
    pXML = udStrSkipWhiteSpace(pXML + udStrMatchBrace(pXML), nullptr, pLineNumber);
  // Skip comment
  if (udStrBeginsWith(pXML, "<!--"))
  {
    pXML = udStrstr(pXML, 0, "-->");
    UD_ERROR_NULL(pXML, udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 3, nullptr, pLineNumber);
  }

  UD_ERROR_IF(*pXML != '<', udR_ParseError);
  result = SetElement();
  UD_ERROR_HANDLE();
  pElement = AsElement();
  pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber);

  // Get the element name
  udStrchr(pXML, " />\r\n", &len);
  UD_ERROR_IF(len < 1, udR_ParseError); // Not going to entertain unnamed elements
  pElement->pName = udStrndup(pXML, len);
  UD_ERROR_NULL(pElement->pName, udR_MemoryAllocationFailure);
  pXML = udStrSkipWhiteSpace(pXML + len, nullptr, pLineNumber);
  selfClosingTag = false;
  while (*pXML != '>')
  {
    // While still inside the element tag, accept and parse attributes
    udStrchr(pXML, " =/>", &len);
    if (pXML[len] == '=')
    {
      // Found an attribute
      udValueObject::KVPair *pKVP = pElement->attributes.PushBack();
      UD_ERROR_NULL(pKVP, udR_MemoryAllocationFailure);
      pKVP->pKey = udStrndup(pXML, len);
      pXML = udStrSkipWhiteSpace(pXML + len + 1, nullptr, pLineNumber);
      result = pKVP->value.Parse(pXML, &charCount);
      UD_ERROR_HANDLE();
      pXML = udStrSkipWhiteSpace(pXML + charCount, nullptr, pLineNumber);
    }
    else if (*pXML == '/')
    {
      selfClosingTag = true;
      pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber);
    }
    else
    {
      UD_ERROR_SET(udR_ParseError);
    }
  }
  pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber); // Skip the >
  if (!selfClosingTag)
  {
    while (*pXML && !udStrBeginsWith(pXML, "</"))
    {
      udValue *pValue;
      result = pElement->children.PushBack(&pValue);
      UD_ERROR_HANDLE();
      pValue->Clear();
      if (*pXML == '<')
      {
        int lineCount;
        result = pValue->Parse(pXML, &charCount, &lineCount);
        UD_ERROR_HANDLE();
        if (pLineNumber)
          *pLineNumber += lineCount - 1;
        pXML = udStrSkipWhiteSpace(pXML + charCount, nullptr, pLineNumber); // Skip </
      }
      else
      {
        udStrchr(pXML, "<>", &len);
        while (len > 1 && (pXML[len] == ' ' || pXML[len] == '\t' || pXML[len] == '\r' || pXML[len] == '\n'))
          --len;
        pValue->u.pStr = udStrndup(pXML, len);
        UD_ERROR_NULL(pValue->u.pStr, udR_MemoryAllocationFailure);
        pValue->type = udVT_String;
        pXML = udStrSkipWhiteSpace(pXML + len, nullptr, pLineNumber);
      }
    }
    UD_ERROR_IF(!*pXML, udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 2, nullptr, pLineNumber); // Skip </
    UD_ERROR_IF(!udStrBeginsWith(pXML, pElement->pName), udR_ParseError); // Check for closing element name
    pXML = udStrSkipWhiteSpace(pXML + udStrlen(pElement->pName), nullptr, pLineNumber); // Skip </
    UD_ERROR_IF(*pXML != '>', udR_ParseError);
    pXML = udStrSkipWhiteSpace(pXML + 1, nullptr, pLineNumber); // Skip >
  }

epilogue:
  if (pCharCount)
    *pCharCount = int(pXML - pStartPointer);
  return result;
}
