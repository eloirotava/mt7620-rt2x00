// SPDX-License-Identifier: GPL-2.0
/*
 * regdump: le os registradores do wifi interno do MT7620 (MAC, BBP, BBP
 * DCOC/GLRT e RF dos 8 bancos) em /proc/regdump, sem depender do driver
 * carregado (rt2800soc ou rt2860v2).  Serve para comparar o estado que
 * cada driver deixa no radio.
 *
 * Nao pega o lock do driver: rodar com o radio parado (sem trafego).
 * A faixa 0x1700-0x17ff (contadores que zeram na leitura e o
 * TX_STA_FIFO) fica de fora.
 */
#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#define WMAC_BASE	0x10180000
#define WMAC_SIZE	0x2000

#define RF_CSR_CFG	0x0500
#define BBP_CSR_CFG	0x101c

static void __iomem *base;

static u32 rd(u32 off) { return readl(base + off); }
static void wr(u32 off, u32 v) { writel(v, base + off); }

static int wait_bit(u32 off, u32 busy, u32 *reg)
{
	int i;

	for (i = 0; i < 100; i++) {
		*reg = rd(off);
		if (!(*reg & busy))
			return 0;
		udelay(20);
	}
	return -ETIMEDOUT;
}

static int bbp_read(u8 word)
{
	u32 reg;

	if (wait_bit(BBP_CSR_CFG, BIT(17), &reg))
		return -1;
	/* REGNUM | BUSY | READ_CONTROL | BBP_RW_MODE */
	wr(BBP_CSR_CFG, (word << 8) | BIT(17) | BIT(16) | BIT(19));
	if (wait_bit(BBP_CSR_CFG, BIT(17), &reg))
		return -1;
	return reg & 0xff;
}

static int bbp_write(u8 word, u8 val)
{
	u32 reg;

	if (wait_bit(BBP_CSR_CFG, BIT(17), &reg))
		return -1;
	wr(BBP_CSR_CFG, val | (word << 8) | BIT(17) | BIT(19));
	return 0;
}

static int rf_read(unsigned int word)
{
	u32 reg;

	if (wait_bit(RF_CSR_CFG, BIT(0), &reg))
		return -1;
	/* REGNUM_MT7620 (bits 16-25) | BUSY_MT7620; WRITE=0 */
	wr(RF_CSR_CFG, (word << 16) | BIT(0));
	if (wait_bit(RF_CSR_CFG, BIT(0), &reg))
		return -1;
	return (reg >> 8) & 0xff;
}

static int rf_write(unsigned int word, u8 val)
{
	u32 reg;

	if (wait_bit(RF_CSR_CFG, BIT(0), &reg))
		return -1;
	wr(RF_CSR_CFG, (word << 16) | (val << 8) | BIT(4) | BIT(0));
	return 0;
}

/* bancos indiretos do BBP: indice em idx, dado em dat; restaura o indice */
static void dump_indexed(struct seq_file *m, const char *name, u8 idx, u8 dat)
{
	int old = bbp_read(idx), i, v;

	for (i = 0; i < 256; i++) {
		bbp_write(idx, i);
		v = bbp_read(dat);
		seq_printf(m, "%s %03d %02x\n", name, i, v & 0xff);
	}
	if (old >= 0)
		bbp_write(idx, old);
}

static int regdump_show(struct seq_file *m, void *v)
{
	static const u32 ranges[][2] = {
		{ 0x0000, 0x0800 },
		{ 0x1000, 0x1700 },
	};
	int i, b, r, val;
	u32 off;

	for (i = 0; i < ARRAY_SIZE(ranges); i++)
		for (off = ranges[i][0]; off < ranges[i][1]; off += 4)
			seq_printf(m, "mac %04x %08x\n", off, rd(off));

	for (r = 0; r < 256; r++) {
		val = bbp_read(r);
		seq_printf(m, "bbp %03d %02x%s\n", r, val & 0xff,
			   val < 0 ? " timeout" : "");
	}
	dump_indexed(m, "dcoc", 158, 159);
	dump_indexed(m, "glrt", 195, 196);

	for (b = 0; b < 8; b++)
		for (r = 0; r < 64; r++) {
			val = rf_read(r | (b << 6));
			seq_printf(m, "rf %d %02d %02x%s\n", b, r, val & 0xff,
				   val < 0 ? " timeout" : "");
		}
	return 0;
}

/*
 * Escrita, uma por linha, no mesmo formato da leitura (hex):
 *   mac <off> <val32> | bbp <reg> <val> | dcoc|glrt <reg> <val> |
 *   rf <banco> <reg> <val>
 * Os numeros de bbp/dcoc/glrt/rf sao decimais, como na saida.
 */
static ssize_t regdump_write(struct file *file, const char __user *ubuf,
			     size_t len, loff_t *ppos)
{
	char buf[64], name[8];
	unsigned int a, b, c;
	int n, ret = 0;

	if (len >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, len))
		return -EFAULT;
	buf[len] = 0;

	n = sscanf(buf, "%7s", name);
	if (n != 1)
		return -EINVAL;
	if (!strcmp(name, "mac") && sscanf(buf, "%*s %x %x", &a, &b) == 2 &&
	    a < WMAC_SIZE && !(a & 3))
		wr(a, b);
	else if (!strcmp(name, "bbp") && sscanf(buf, "%*s %u %x", &a, &b) == 2 &&
		 a < 256)
		ret = bbp_write(a, b);
	else if ((!strcmp(name, "dcoc") || !strcmp(name, "glrt")) &&
		 sscanf(buf, "%*s %u %x", &a, &b) == 2 && a < 256) {
		u8 idx = name[0] == 'd' ? 158 : 195;

		ret = bbp_write(idx, a) || bbp_write(idx + 1, b);
	} else if (!strcmp(name, "rf") &&
		   sscanf(buf, "%*s %u %u %x", &a, &b, &c) == 3 && a < 8 && b < 64)
		ret = rf_write(b | (a << 6), c);
	else
		return -EINVAL;
	return ret ? -ETIMEDOUT : len;
}

static int regdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, regdump_show, NULL);
}

static const struct proc_ops regdump_ops = {
	.proc_open	= regdump_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
	.proc_write	= regdump_write,
};

static int __init regdump_init(void)
{
	base = ioremap(WMAC_BASE, WMAC_SIZE);
	if (!base)
		return -ENOMEM;
	if (!proc_create("regdump", 0600, NULL, &regdump_ops)) {
		iounmap(base);
		return -ENOMEM;
	}
	return 0;
}

static void __exit regdump_exit(void)
{
	remove_proc_entry("regdump", NULL);
	iounmap(base);
}

module_init(regdump_init);
module_exit(regdump_exit);
MODULE_LICENSE("GPL");
