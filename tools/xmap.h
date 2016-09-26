#ifndef XMAP_H
#define XMAP_H

#define TN_ID   0
#define TN_NAME 1

#define DOS_MAGIC 0x5a4d
#define PE_MAGIC  0x00004550

#include <stddef.h>
#include <stdint.h>

struct mz {
	uint16_t e_magic;
	uint16_t e_cblp;
	uint16_t e_cp;
	uint16_t e_crlc;
	uint16_t e_cparhdr;
	uint16_t e_minalloc;
	uint16_t e_maxalloc;
	uint16_t e_ss;
	uint16_t e_sp;
	uint16_t e_csum;
	uint16_t e_ip;
	uint16_t e_cs;
	uint16_t e_lfarlc;
	uint16_t e_ovno;
};

struct dos {
	struct mz mz;
	uint16_t e_res[4];
	uint16_t e_oemid;
	uint16_t e_oeminfo;
	uint16_t e_res2[10];
	uint16_t e_lfanew;
};

struct pehdr {
	uint32_t f_magic;
	uint16_t f_mach;
	uint16_t f_nscns;
	uint32_t f_timdat;
	uint32_t f_symptr;
	uint32_t f_nsyms;
	uint16_t f_opthdr;
	uint16_t f_flags;
};

struct coffopthdr {
	uint16_t o_magic;
	uint16_t o_vstamp;
	uint32_t o_tsize;
	uint32_t o_dsize;
	uint32_t o_bsize;
	uint32_t o_entry;
	uint32_t o_text;
	uint32_t o_data;
	uint32_t o_image;
};

struct peddir {
	uint32_t d_texp;
	uint32_t d_nexp;
	uint32_t d_timp;
	uint32_t d_nimp;
	uint32_t d_tres;
	uint32_t d_nres;
	uint32_t d_texc;
	uint32_t d_nexc;
	uint32_t d_tcrt;
	uint32_t d_ncrt;
	uint32_t d_trel;
	uint32_t d_nrel;
	uint32_t d_dbg;
	uint32_t d_ndbg;
	uint32_t d_arch;
	uint32_t d_narch;
	uint32_t d_glbl;
	uint32_t d_gzero;
	uint32_t d_ttls;
	uint32_t d_ntls;
	uint32_t d_tcfg;
	uint32_t d_ncfg;
	uint32_t d_bimp;
	uint32_t d_nbimp;
	uint32_t d_tiaddr;
	uint32_t d_niaddr;
	uint32_t d_did;
	uint32_t d_ndid;
	uint32_t d_crth;
	uint32_t d_ncrth;
	uint64_t d_end;
};

struct peopthdr {
	struct coffopthdr o_chdr;
	uint32_t o_alnsec;
	uint32_t o_alnfile;
	uint16_t o_osmajor;
	uint16_t o_osminor;
	uint16_t o_imajor;
	uint16_t o_iminor;
	uint16_t o_smajor;
	uint16_t o_sminor;
	uint32_t o_vw32;
	uint32_t o_isize;
	uint32_t o_hsize;
	uint32_t o_chksum;
	uint16_t o_sub;
	uint16_t o_dllflags;
	uint32_t o_sres;
	uint32_t o_scomm;
	uint32_t o_hres;
	uint32_t o_hcomm;
	uint32_t o_ldflags;
	uint32_t o_nrvasz;
	struct peddir o_ddir;
};

#define RT_UNKNOWN      0
#define RT_CURSOR       1
#define RT_BITMAP       2
#define RT_ICON         3
#define RT_MENU         4
#define RT_DIALOG       5
#define RT_STRING       6
#define RT_FONTDIR      7
#define RT_FONT         8
#define RT_ACCELERATOR  9
#define RT_RCDATA       10
#define RT_MESSAGETABLE 11
#define RT_GROUP_CURSOR 12
#define RT_GROUP_ICON   14
#define RT_VERSION      16
#define RT_DLGINCLUDE   17
#define RT_PLUGPLAY     19
#define RT_VXD          20
#define RT_ANICURSOR    21
#define RT_ANIICON      22

struct rsrcdir {
	uint32_t r_flags;
	uint32_t r_timdat;
	uint16_t r_major;
	uint16_t r_minor;
	uint16_t r_nnment;
	uint16_t r_nident;
};

struct rsrcditem {
	uint32_t r_id;
	uint32_t r_rva;
};

#define SF_CODE   0x00000020
#define SF_DATA   0x00000040
#define SF_BSS    0x00000080
#define SF_DIRECT 0x04000000
#define SF_FIXED  0x08000000
#define SF_SHARE  0x10000000
#define SF_EXEC   0x20000000
#define SF_READ   0x40000000
#define SF_WRITE  0x80000000

struct sechdr {
	char s_name[8];
	uint32_t s_paddr;
	uint32_t s_vaddr;
	uint32_t s_size;
	uint32_t s_scnptr;
	uint32_t s_relptr;
	uint32_t s_lnnoptr;
	uint16_t s_nreloc;
	uint16_t s_nlnno;
	uint32_t s_flags;
};

#define XT_UNKNOWN 0
#define XT_MZ      1
#define XT_DOS     2
#define XT_PE      3
#define XT_PEOPT   4

struct xfile {
	unsigned type;
	unsigned bits;
	char *data;
	size_t size;
	struct mz *mz;
	struct dos *dos;
	struct pehdr *pe;
	struct peopthdr *peopt;
	unsigned nrvasz;
	unsigned nrvan;
	struct sechdr *sec;
	struct rsrcdir *rsrc;
};

int xstat(struct xfile *this, char *data, size_t size);
int xmap(const char *name, int prot, int *fd, char **map, size_t *mapsz);
int xunmap(int fd, char *map, size_t mapsz);

#endif
