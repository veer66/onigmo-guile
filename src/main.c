/*
 *
 * Copyright (c) 2016 Vee Satayamas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include <libguile.h>
#include <oniguruma.h>

static scm_t_bits re_tag;

size_t
gc_free_re(SCM re)
{
  SCM_CRITICAL_SECTION_START;
  regex_t *_re = (regex_t *) SCM_SMOB_DATA(re);
  onig_free(_re);
  SCM_CRITICAL_SECTION_END;
  return 0;
}

SCM
make_re(SCM _pat)
{
  SCM re_smob;
  regex_t *re;
  int r;
  size_t pat_len;
  UChar *pat = (UChar *) scm_to_utf32_stringn(_pat, &pat_len);
  OnigErrorInfo e_info;
  OnigEncoding enc = ONIG_ENCODING_UTF32_LE;

  r = onig_new(&re,
	       pat,
	       pat + pat_len*4,
	       ONIG_OPTION_NONE,
	       enc,
	       ONIG_SYNTAX_DEFAULT,
	       &e_info);
  free(pat);

  if (r != ONIG_NORMAL)
    {
      scm_error (SCM_BOOL_F,
		 "make-re",
		 "cannot interpret pattern",
		 SCM_BOOL_F,
		 SCM_BOOL_F);
      return SCM_UNSPECIFIED;
    }

  SCM_NEWSMOB (re_smob, re_tag, re);
  return re_smob;
}

static
SCM
region_to_text_ranges(OnigRegion *region)
{
  SCM text_ranges = SCM_EOL;
  SCM text_range;
  int i;

  for (i = region->num_regs - 1; i >= 0; i--)
    {
      text_range = scm_cons(scm_from_int(region->beg[i] / 4),
			    scm_from_int(region->end[i] / 4));
      text_ranges = scm_cons(text_range,
			     text_ranges);      
    }
  return text_ranges;
}

SCM
onig_re_search(SCM re, SCM str) 
{
  regex_t *_re = (regex_t *) SCM_SMOB_DATA(re);
  OnigEncoding enc;
  int r;
  unsigned char *start, *end, *range;
  OnigRegion *region;
  UChar *_str;
  size_t len;

  enc = ONIG_ENCODING_UTF32_LE;
  _str = (UChar *)scm_to_utf32_stringn(str, &len);
  
  end = _str + len * 4;
  region = onig_region_new();
  start = _str;
  range = end;
  
  r = onig_search(_re,
  		  _str,
  		  end,
  		  start,
  		  range,
  		  region,
  		  ONIG_OPTION_NONE);

  if (r >= 0)
    {
      return region_to_text_ranges(region);
    }
  else if (r == ONIG_MISMATCH)
    {
      return SCM_EOL;
    }
  else
    {
      scm_error (SCM_BOOL_F,
		 "make-re",
		 "cannot interpret pattern",
		 SCM_BOOL_F,
		 SCM_BOOL_F);
    }
  onig_region_free(region, 1);
  return SCM_UNSPECIFIED;
}

void
scm_init_onig_re(void)
{
  re_tag = scm_make_smob_type("onig-re", sizeof (regex_t));
  scm_set_smob_free (re_tag, gc_free_re);
  scm_c_define_gsubr("make-re", 1, 0, 0, make_re);
  scm_c_define_gsubr("re-search", 2, 0, 0, onig_re_search);
}

void
scm_init_onig()
{
  scm_init_onig_re();
}
