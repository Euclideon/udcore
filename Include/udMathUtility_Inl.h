

template <typename T>
udVector3<T> udPerpendicular3(const udVector3<T> &axis)
{
  //The idea here is to choose two non-zero elements, negate and switch so that axis dot perp == 0.
  //Here we simply choose the largest two elements (in magnitude) to try and avoid any close-to-zero values.
  udVector3<T> perp = {};

  int minInd = (udAbs(axis[0]) < udAbs(axis[1])) ? 0 : 1;
  if (udAbs(axis[2]) < udAbs(axis[minInd]))
    minInd = 2;

  int firstInd = (minInd + 1) % 3;
  int secondInd = (minInd + 2) % 3;

  perp[firstInd] = -axis[secondInd];
  perp[secondInd] = axis[firstInd];

  return perp;
}

template <typename T>
bool udIsRotatedAxisStillAxisAligned(const udQuaternion<T> &q, const udVector3<T> &extentsIn, udVector3<T> &extentsOut, const T epsilon)
{
  extentsOut = udVector3<T>::zero();
  bool result = true;
  for (int i = 0; i < 3; ++i)
  {
    udDouble3 v = {};
    v[i] = extentsIn[i];
    v = q.apply(v);

    int nonZeroIndex = -1;
    bool isAA = true;
    for (int j = 0; j < 3; ++j)
    {
      if (udAbs(v[j]) <= epsilon)
        continue;
      if (nonZeroIndex != -1)
      {
        isAA = false;
        break;
      }
      nonZeroIndex = j;
    }
    if (!isAA)
    {
      result = false;
      break;
    }
    if (nonZeroIndex > -1)
    {
      (extentsOut)[nonZeroIndex] = udAbs((extentsIn)[i]);
      if (v[nonZeroIndex] < 0.0)
        (extentsOut)[nonZeroIndex] = -(extentsOut)[nonZeroIndex];
    }
  }
  return result;
}
