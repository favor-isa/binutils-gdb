#define TC_FAVOR 1
#define TARGET_BYTES_BIG_ENDIAN 0
#define WORKING_DOT_WORD

#define TARGET_FORMAT "elf32-favor"

#define TARGET_ARCH bfd_arch_favor

#define md_undefined_symbol(NAME) 0

static inline int _favor_fatal(const char *str) {
    as_fatal("%s", str);
    return 0;
}

// #define md_estimate_size_before_relax(A, B) (_favor_fatal (_("estimate size\n")))
// #define md_convert_frag(B, S, F)            (_favor_fatal (_("convert_frag\n")))

// 4 bytes offset?
#define md_pcrel_from(FIX) 						\
	((FIX)->fx_where + (FIX)->fx_frag->fr_address)

#define md_section_align(SEGMENT, SIZE)     (SIZE)