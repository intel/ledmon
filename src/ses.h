/* <COPYRIGHT year="2009,2011,2012" comp="Intel Corporation" desc="Intel(R) Enclosure LED Utilities" /> */

/* Size of buffer for SES-2 Messages. */
#define SES_ALLOC_BUFF 4096

#define ENCL_CFG_DIAG_STATUS		0x01
#define ENCL_CTRL_DIAG_STATUS		0x02
#define ENCL_CTRL_DIAG_CFG			0x02
#define ENCL_EL_DESCR_STATUS		0x07
#define ENCL_ADDITIONAL_EL_STATUS		0x0a
#define SCSI_PROTOCOL_SAS			6

#define SES_DEVICE_SLOT			0x01
#define SES_ARRAY_DEVICE_SLOT	0x17

static inline void _clr_msg(unsigned char *u)
{
	u[0] = u[1] = u[2] = u[3] = 0;
}
static inline void _set_abrt(unsigned char *u)
{
	u[1] |= (1 << 0);
}
static inline void _set_rebuild(unsigned char *u)
{
	u[1] |= (1 << 1);
}
static inline void _set_ifa(unsigned char *u)
{
	u[1] |= (1 << 2);
}
static inline void _set_ica(unsigned char *u)
{
	u[1] |= (1 << 3);
}
static inline void _set_cons_check(unsigned char *u)
{
	u[1] |= (1 << 4);
}
static inline void _set_hspare(unsigned char *u)
{
	u[1] |= (1 << 5);
}
static inline void _set_rsvd_dev(unsigned char *u)
{
	u[1] |= (1 << 6);
}
static inline void _set_ok(unsigned char *u)
{
	u[1] |= (1 << 7);
}
static inline void _set_ident(unsigned char *u)
{
	u[2] |= (1 << 1);
}
static inline void _clr_ident(unsigned char *u)
{
	u[2] &= ~(1 << 1);
}
static inline void _set_rm(unsigned char *u)
{
	u[2] |= (1 << 2);
}
static inline void _set_ins(unsigned char *u)
{
	u[2] |= (1 << 3);
}
static inline void _set_miss(unsigned char *u)
{
	u[2] |= (1 << 4);
}
static inline void _set_dnr(unsigned char *u)
{
	u[2] |= (1 << 6);
}
static inline void _set_actv(unsigned char *u)
{
	u[2] |= (1 << 7);
}
static inline void _set_enbb(unsigned char *u)
{
	u[3] |= (1 << 2);
}
static inline void _set_enba(unsigned char *u)
{
	u[3] |= (1 << 3);
}
static inline void _set_off(unsigned char *u)
{
	u[3] |= (1 << 4);
}
static inline void _set_fault(unsigned char *u)
{
	u[3] |= (1 << 5);
}


struct ses_pages {
	unsigned char *page1;
	int page1_len;
	unsigned char *page2;
	int page2_len;
	unsigned char *page10;
	int page10_len;
	unsigned char *page1_types;
	int page1_types_len;
	int components;
};
