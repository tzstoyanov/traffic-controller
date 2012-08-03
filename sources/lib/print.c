
/* Convert an integer to a decimal string without using much stack
   space.  Return a pointer to the part of the buffer to use.  We this
   instead of sprintf because sprintf will require too much stack
   space.  */


 char *
print_int (int val, char *buf, int buflen, size_t *print_len)
{
  int is_negative;
  int i;
  unsigned int uval;

  uval = (unsigned int) val;
  if (val >= 0)
    is_negative = 0;
  else
    {
      is_negative = 1;
      uval = - uval;
    }

  i = buflen;
  do
    {
      --i;
      buf[i] = '0' + (uval % 10);
      uval /= 10;
    }
  while (uval != 0 && i > 0);

  if (is_negative)
    {
      if (i > 0)
	--i;
      buf[i] = '-';
    }

  *print_len = buflen - i;
  return buf + i;
}


/* Convert an integer to a decimal string without using much stack
   space.  Return a pointer to the part of the buffer to use.  We this
   instead of sprintf because sprintf will require too much stack
   space.  */


 char *
print_hex(unsigned short uval, char *buf, int buflen, size_t *print_len)
{
  int i;

  i = buflen;
  do
    {
      --i;
      buf[i] = *("0123456789ABCFEF" + (uval & 15));
      uval >> 4;
    }
  while (uval != 0 && i > 0);

  *print_len = buflen - i;
  return buf + i;
}

