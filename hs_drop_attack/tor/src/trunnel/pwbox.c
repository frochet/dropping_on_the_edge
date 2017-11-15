/* pwbox.c -- generated by Trunnel v1.4.3.
 * https://gitweb.torproject.org/trunnel.git
 * You probably shouldn't edit this file.
 */
#include <stdlib.h>
#include "trunnel-impl.h"

#include "pwbox.h"

#define TRUNNEL_SET_ERROR_CODE(obj) \
  do {                              \
    (obj)->trunnel_error_code_ = 1; \
  } while (0)

#if defined(__COVERITY__) || defined(__clang_analyzer__)
/* If we're runnning a static analysis tool, we don't want it to complain
 * that some of our remaining-bytes checks are dead-code. */
int pwbox_deadcode_dummy__ = 0;
#define OR_DEADCODE_DUMMY || pwbox_deadcode_dummy__
#else
#define OR_DEADCODE_DUMMY
#endif

#define CHECK_REMAINING(nbytes, label)                           \
  do {                                                           \
    if (remaining < (nbytes) OR_DEADCODE_DUMMY) {                \
      goto label;                                                \
    }                                                            \
  } while (0)

pwbox_encoded_t *
pwbox_encoded_new(void)
{
  pwbox_encoded_t *val = trunnel_calloc(1, sizeof(pwbox_encoded_t));
  if (NULL == val)
    return NULL;
  val->fixedbytes0 = PWBOX0_CONST0;
  val->fixedbytes1 = PWBOX0_CONST1;
  return val;
}

/** Release all storage held inside 'obj', but do not free 'obj'.
 */
static void
pwbox_encoded_clear(pwbox_encoded_t *obj)
{
  (void) obj;
  TRUNNEL_DYNARRAY_WIPE(&obj->skey_header);
  TRUNNEL_DYNARRAY_CLEAR(&obj->skey_header);
  TRUNNEL_DYNARRAY_WIPE(&obj->data);
  TRUNNEL_DYNARRAY_CLEAR(&obj->data);
}

void
pwbox_encoded_free(pwbox_encoded_t *obj)
{
  if (obj == NULL)
    return;
  pwbox_encoded_clear(obj);
  trunnel_memwipe(obj, sizeof(pwbox_encoded_t));
  trunnel_free_(obj);
}

uint32_t
pwbox_encoded_get_fixedbytes0(pwbox_encoded_t *inp)
{
  return inp->fixedbytes0;
}
int
pwbox_encoded_set_fixedbytes0(pwbox_encoded_t *inp, uint32_t val)
{
  if (! ((val == PWBOX0_CONST0))) {
     TRUNNEL_SET_ERROR_CODE(inp);
     return -1;
  }
  inp->fixedbytes0 = val;
  return 0;
}
uint32_t
pwbox_encoded_get_fixedbytes1(pwbox_encoded_t *inp)
{
  return inp->fixedbytes1;
}
int
pwbox_encoded_set_fixedbytes1(pwbox_encoded_t *inp, uint32_t val)
{
  if (! ((val == PWBOX0_CONST1))) {
     TRUNNEL_SET_ERROR_CODE(inp);
     return -1;
  }
  inp->fixedbytes1 = val;
  return 0;
}
uint8_t
pwbox_encoded_get_header_len(pwbox_encoded_t *inp)
{
  return inp->header_len;
}
int
pwbox_encoded_set_header_len(pwbox_encoded_t *inp, uint8_t val)
{
  inp->header_len = val;
  return 0;
}
size_t
pwbox_encoded_getlen_skey_header(const pwbox_encoded_t *inp)
{
  return TRUNNEL_DYNARRAY_LEN(&inp->skey_header);
}

uint8_t
pwbox_encoded_get_skey_header(pwbox_encoded_t *inp, size_t idx)
{
  return TRUNNEL_DYNARRAY_GET(&inp->skey_header, idx);
}

int
pwbox_encoded_set_skey_header(pwbox_encoded_t *inp, size_t idx, uint8_t elt)
{
  TRUNNEL_DYNARRAY_SET(&inp->skey_header, idx, elt);
  return 0;
}
int
pwbox_encoded_add_skey_header(pwbox_encoded_t *inp, uint8_t elt)
{
#if SIZE_MAX >= UINT8_MAX
  if (inp->skey_header.n_ == UINT8_MAX)
    goto trunnel_alloc_failed;
#endif
  TRUNNEL_DYNARRAY_ADD(uint8_t, &inp->skey_header, elt, {});
  return 0;
 trunnel_alloc_failed:
  TRUNNEL_SET_ERROR_CODE(inp);
  return -1;
}

uint8_t *
pwbox_encoded_getarray_skey_header(pwbox_encoded_t *inp)
{
  return inp->skey_header.elts_;
}
int
pwbox_encoded_setlen_skey_header(pwbox_encoded_t *inp, size_t newlen)
{
  uint8_t *newptr;
#if UINT8_MAX < SIZE_MAX
  if (newlen > UINT8_MAX)
    goto trunnel_alloc_failed;
#endif
  newptr = trunnel_dynarray_setlen(&inp->skey_header.allocated_,
                 &inp->skey_header.n_, inp->skey_header.elts_, newlen,
                 sizeof(inp->skey_header.elts_[0]), (trunnel_free_fn_t) NULL,
                 &inp->trunnel_error_code_);
  if (newptr == NULL)
    goto trunnel_alloc_failed;
  inp->skey_header.elts_ = newptr;
  return 0;
 trunnel_alloc_failed:
  TRUNNEL_SET_ERROR_CODE(inp);
  return -1;
}
size_t
pwbox_encoded_getlen_iv(const pwbox_encoded_t *inp)
{
  (void)inp;  return 16;
}

uint8_t
pwbox_encoded_get_iv(const pwbox_encoded_t *inp, size_t idx)
{
  trunnel_assert(idx < 16);
  return inp->iv[idx];
}

int
pwbox_encoded_set_iv(pwbox_encoded_t *inp, size_t idx, uint8_t elt)
{
  trunnel_assert(idx < 16);
  inp->iv[idx] = elt;
  return 0;
}

uint8_t *
pwbox_encoded_getarray_iv(pwbox_encoded_t *inp)
{
  return inp->iv;
}
size_t
pwbox_encoded_getlen_data(const pwbox_encoded_t *inp)
{
  return TRUNNEL_DYNARRAY_LEN(&inp->data);
}

uint8_t
pwbox_encoded_get_data(pwbox_encoded_t *inp, size_t idx)
{
  return TRUNNEL_DYNARRAY_GET(&inp->data, idx);
}

int
pwbox_encoded_set_data(pwbox_encoded_t *inp, size_t idx, uint8_t elt)
{
  TRUNNEL_DYNARRAY_SET(&inp->data, idx, elt);
  return 0;
}
int
pwbox_encoded_add_data(pwbox_encoded_t *inp, uint8_t elt)
{
  TRUNNEL_DYNARRAY_ADD(uint8_t, &inp->data, elt, {});
  return 0;
 trunnel_alloc_failed:
  TRUNNEL_SET_ERROR_CODE(inp);
  return -1;
}

uint8_t *
pwbox_encoded_getarray_data(pwbox_encoded_t *inp)
{
  return inp->data.elts_;
}
int
pwbox_encoded_setlen_data(pwbox_encoded_t *inp, size_t newlen)
{
  uint8_t *newptr;
  newptr = trunnel_dynarray_setlen(&inp->data.allocated_,
                 &inp->data.n_, inp->data.elts_, newlen,
                 sizeof(inp->data.elts_[0]), (trunnel_free_fn_t) NULL,
                 &inp->trunnel_error_code_);
  if (newptr == NULL)
    goto trunnel_alloc_failed;
  inp->data.elts_ = newptr;
  return 0;
 trunnel_alloc_failed:
  TRUNNEL_SET_ERROR_CODE(inp);
  return -1;
}
size_t
pwbox_encoded_getlen_hmac(const pwbox_encoded_t *inp)
{
  (void)inp;  return 32;
}

uint8_t
pwbox_encoded_get_hmac(const pwbox_encoded_t *inp, size_t idx)
{
  trunnel_assert(idx < 32);
  return inp->hmac[idx];
}

int
pwbox_encoded_set_hmac(pwbox_encoded_t *inp, size_t idx, uint8_t elt)
{
  trunnel_assert(idx < 32);
  inp->hmac[idx] = elt;
  return 0;
}

uint8_t *
pwbox_encoded_getarray_hmac(pwbox_encoded_t *inp)
{
  return inp->hmac;
}
const char *
pwbox_encoded_check(const pwbox_encoded_t *obj)
{
  if (obj == NULL)
    return "Object was NULL";
  if (obj->trunnel_error_code_)
    return "A set function failed on this object";
  if (! (obj->fixedbytes0 == PWBOX0_CONST0))
    return "Integer out of bounds";
  if (! (obj->fixedbytes1 == PWBOX0_CONST1))
    return "Integer out of bounds";
  if (TRUNNEL_DYNARRAY_LEN(&obj->skey_header) != obj->header_len)
    return "Length mismatch for skey_header";
  return NULL;
}

ssize_t
pwbox_encoded_encoded_len(const pwbox_encoded_t *obj)
{
  ssize_t result = 0;

  if (NULL != pwbox_encoded_check(obj))
     return -1;


  /* Length of u32 fixedbytes0 IN [PWBOX0_CONST0] */
  result += 4;

  /* Length of u32 fixedbytes1 IN [PWBOX0_CONST1] */
  result += 4;

  /* Length of u8 header_len */
  result += 1;

  /* Length of u8 skey_header[header_len] */
  result += TRUNNEL_DYNARRAY_LEN(&obj->skey_header);

  /* Length of u8 iv[16] */
  result += 16;

  /* Length of u8 data[] */
  result += TRUNNEL_DYNARRAY_LEN(&obj->data);

  /* Length of u8 hmac[32] */
  result += 32;
  return result;
}
int
pwbox_encoded_clear_errors(pwbox_encoded_t *obj)
{
  int r = obj->trunnel_error_code_;
  obj->trunnel_error_code_ = 0;
  return r;
}
ssize_t
pwbox_encoded_encode(uint8_t *output, size_t avail, const pwbox_encoded_t *obj)
{
  ssize_t result = 0;
  size_t written = 0;
  uint8_t *ptr = output;
  const char *msg;
#ifdef TRUNNEL_CHECK_ENCODED_LEN
  const ssize_t encoded_len = pwbox_encoded_encoded_len(obj);
#endif
  int enforce_avail = 0;
  const size_t avail_orig = avail;

  if (NULL != (msg = pwbox_encoded_check(obj)))
    goto check_failed;

#ifdef TRUNNEL_CHECK_ENCODED_LEN
  trunnel_assert(encoded_len >= 0);
#endif

  /* Encode u32 fixedbytes0 IN [PWBOX0_CONST0] */
  trunnel_assert(written <= avail);
  if (avail - written < 4)
    goto truncated;
  trunnel_set_uint32(ptr, trunnel_htonl(obj->fixedbytes0));
  written += 4; ptr += 4;

  /* Encode u32 fixedbytes1 IN [PWBOX0_CONST1] */
  trunnel_assert(written <= avail);
  if (avail - written < 4)
    goto truncated;
  trunnel_set_uint32(ptr, trunnel_htonl(obj->fixedbytes1));
  written += 4; ptr += 4;

  /* Encode u8 header_len */
  trunnel_assert(written <= avail);
  if (avail - written < 1)
    goto truncated;
  trunnel_set_uint8(ptr, (obj->header_len));
  written += 1; ptr += 1;

  /* Encode u8 skey_header[header_len] */
  {
    size_t elt_len = TRUNNEL_DYNARRAY_LEN(&obj->skey_header);
    trunnel_assert(obj->header_len == elt_len);
    trunnel_assert(written <= avail);
    if (avail - written < elt_len)
      goto truncated;
    memcpy(ptr, obj->skey_header.elts_, elt_len);
    written += elt_len; ptr += elt_len;
  }

  /* Encode u8 iv[16] */
  trunnel_assert(written <= avail);
  if (avail - written < 16)
    goto truncated;
  memcpy(ptr, obj->iv, 16);
  written += 16; ptr += 16;
  {

    /* Encode u8 data[] */
    {
      size_t elt_len = TRUNNEL_DYNARRAY_LEN(&obj->data);
      trunnel_assert(written <= avail);
      if (avail - written < elt_len)
        goto truncated;
      memcpy(ptr, obj->data.elts_, elt_len);
      written += elt_len; ptr += elt_len;
    }
    trunnel_assert(written <= avail);
    if (avail - written < 32)
      goto truncated;
    avail = written + 32;
    enforce_avail = 1;
  }

  /* Encode u8 hmac[32] */
  trunnel_assert(written <= avail);
  if (avail - written < 32) {
    if (avail_orig - written < 32)
      goto truncated;
    else
      goto check_failed;
  }
  memcpy(ptr, obj->hmac, 32);
  written += 32; ptr += 32;


  trunnel_assert(ptr == output + written);
  if (enforce_avail && avail != written)
    goto check_failed;
#ifdef TRUNNEL_CHECK_ENCODED_LEN
  {
    trunnel_assert(encoded_len >= 0);
    trunnel_assert((size_t)encoded_len == written);
  }

#endif

  return written;

 truncated:
  result = -2;
  goto fail;
 check_failed:
  (void)msg;
  result = -1;
  goto fail;
 fail:
  trunnel_assert(result < 0);
  return result;
}

/** As pwbox_encoded_parse(), but do not allocate the output object.
 */
static ssize_t
pwbox_encoded_parse_into(pwbox_encoded_t *obj, const uint8_t *input, const size_t len_in)
{
  const uint8_t *ptr = input;
  size_t remaining = len_in;
  ssize_t result = 0;
  (void)result;

  /* Parse u32 fixedbytes0 IN [PWBOX0_CONST0] */
  CHECK_REMAINING(4, truncated);
  obj->fixedbytes0 = trunnel_ntohl(trunnel_get_uint32(ptr));
  remaining -= 4; ptr += 4;
  if (! (obj->fixedbytes0 == PWBOX0_CONST0))
    goto fail;

  /* Parse u32 fixedbytes1 IN [PWBOX0_CONST1] */
  CHECK_REMAINING(4, truncated);
  obj->fixedbytes1 = trunnel_ntohl(trunnel_get_uint32(ptr));
  remaining -= 4; ptr += 4;
  if (! (obj->fixedbytes1 == PWBOX0_CONST1))
    goto fail;

  /* Parse u8 header_len */
  CHECK_REMAINING(1, truncated);
  obj->header_len = (trunnel_get_uint8(ptr));
  remaining -= 1; ptr += 1;

  /* Parse u8 skey_header[header_len] */
  CHECK_REMAINING(obj->header_len, truncated);
  TRUNNEL_DYNARRAY_EXPAND(uint8_t, &obj->skey_header, obj->header_len, {});
  obj->skey_header.n_ = obj->header_len;
  memcpy(obj->skey_header.elts_, ptr, obj->header_len);
  ptr += obj->header_len; remaining -= obj->header_len;

  /* Parse u8 iv[16] */
  CHECK_REMAINING(16, truncated);
  memcpy(obj->iv, ptr, 16);
  remaining -= 16; ptr += 16;
  {
    size_t remaining_after;
    CHECK_REMAINING(32, truncated);
    remaining_after = 32;
    remaining = remaining - 32;

    /* Parse u8 data[] */
    TRUNNEL_DYNARRAY_EXPAND(uint8_t, &obj->data, remaining, {});
    obj->data.n_ = remaining;
    memcpy(obj->data.elts_, ptr, remaining);
    ptr += remaining; remaining -= remaining;
    if (remaining != 0)
      goto fail;
    remaining = remaining_after;
  }

  /* Parse u8 hmac[32] */
  CHECK_REMAINING(32, truncated);
  memcpy(obj->hmac, ptr, 32);
  remaining -= 32; ptr += 32;
  trunnel_assert(ptr + remaining == input + len_in);
  return len_in - remaining;

 truncated:
  return -2;
 trunnel_alloc_failed:
  return -1;
 fail:
  result = -1;
  return result;
}

ssize_t
pwbox_encoded_parse(pwbox_encoded_t **output, const uint8_t *input, const size_t len_in)
{
  ssize_t result;
  *output = pwbox_encoded_new();
  if (NULL == *output)
    return -1;
  result = pwbox_encoded_parse_into(*output, input, len_in);
  if (result < 0) {
    pwbox_encoded_free(*output);
    *output = NULL;
  }
  return result;
}
