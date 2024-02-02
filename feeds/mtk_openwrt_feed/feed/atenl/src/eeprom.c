#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "atenl.h"

#define EEPROM_PART_SIZE 0xFF000
char *eeprom_file;

static int
atenl_create_file(struct atenl *an, bool flash_mode)
{
	char fname[64], buf[1024];
	ssize_t w, len, max_len, total_len = 0;
	int fd_ori, fd, ret;

	/* reserve space for pre-cal data in flash mode */
	if (flash_mode) {
		atenl_dbg("%s: init eeprom with flash / binfile mode\n", __func__);
		max_len = EEPROM_PART_SIZE;
	} else {
		atenl_dbg("%s: init eeprom with efuse / default bin mode\n", __func__);
		max_len = 0x1e00;
	}

	snprintf(fname, sizeof(fname),
		 "/sys/kernel/debug/ieee80211/phy%d/mt76/eeprom",
		 get_band_val(an, 0, phy_idx));
	fd_ori = open(fname, O_RDONLY);
	if (fd_ori < 0)
		return -1;

	fd = open(eeprom_file, O_RDWR | O_CREAT | O_EXCL, 00644);
	if (fd < 0)
		goto out;

	while ((len = read(fd_ori, buf, sizeof(buf))) > 0) {
retry:
		w = write(fd, buf, len);
		if (w > 0) {
			total_len += len;
			continue;
		}

		if (errno == EINTR)
			goto retry;

		perror("write");
		unlink(eeprom_file);
		close(fd);
		fd = -1;
		goto out;
	}

	/* reserve space for pre-cal data in flash mode */
	len = sizeof(buf);
	memset(buf, 0, len);
	while (total_len < max_len) {
		w = write(fd, buf, len);

		if (w > 0) {
			total_len += len;
			continue;
		}

		if (errno != EINTR) {
			perror("write");
			unlink(eeprom_file);
			close(fd);
			fd = -1;
			goto out;
		}
	}


	ret = lseek(fd, 0, SEEK_SET);
	if (ret) {
		close(fd_ori);
		close(fd);
		return ret;
	}

out:
	close(fd_ori);
	return fd;
}

static bool
atenl_eeprom_file_exists(void)
{
	struct stat st;

	return stat(eeprom_file, &st) == 0;
}

static int
atenl_eeprom_init_file(struct atenl *an, bool flash_mode)
{
	int fd;

	if (!atenl_eeprom_file_exists())
		return atenl_create_file(an, flash_mode);

	fd = open(eeprom_file, O_RDWR);
	if (fd < 0)
		perror("open");

	return fd;
}

static void
atenl_eeprom_init_chip_id(struct atenl *an)
{
	an->chip_id = *(u16 *)an->eeprom_data;

	if (is_mt7915(an)) {
		an->adie_id = 0x7975;
	} else if (is_mt7916(an) || is_mt7981(an)) {
		an->adie_id = 0x7976;
	} else if (is_mt7986(an)) {
		bool is_7975 = false;
		u32 val;
		u8 sub_id;

		atenl_reg_read(an, 0x18050000, &val);

		switch (val & 0xf) {
		case MT7975_ONE_ADIE_SINGLE_BAND:
			is_7975 = true;
			/* fallthrough */
		case MT7976_ONE_ADIE_SINGLE_BAND:
			sub_id = 0xa;
			break;
		case MT7976_ONE_ADIE_DBDC:
			sub_id = 0x7;
			break;
		case MT7975_DUAL_ADIE_DBDC:
			is_7975 = true;
			/* fallthrough */
		case MT7976_DUAL_ADIE_DBDC:
		default:
			sub_id = 0xf;
			break;
		}

		an->sub_chip_id = sub_id;
		an->adie_id = is_7975 ? 0x7975 : 0x7976;
	} else if (is_mt7996(an)) {
		/* TODO: parse info if required */
	} else if (is_mt7992(an)) {
		/* TODO: parse info if required */
	}
}

static void
atenl_eeprom_init_max_size(struct atenl *an)
{
	switch (an->chip_id) {
	case 0x7915:
		an->eeprom_size = 3584;
		an->eeprom_prek_offs = 0x62;
		break;
	case 0x7906:
	case 0x7916:
	case 0x7981:
	case 0x7986:
		an->eeprom_size = 4096;
		an->eeprom_prek_offs = 0x19a;
		break;
	case 0x7990:
	case 0x7992:
		an->eeprom_size = 7680;
		an->eeprom_prek_offs = 0x1a5;
	default:
		break;
	}
}

static void
atenl_eeprom_init_band_cap(struct atenl *an)
{
#define EAGLE_BAND_SEL(index)	MT_EE_WIFI_EAGLE_CONF##index##_BAND_SEL
	u8 *eeprom = an->eeprom_data;

	if (is_mt7915(an)) {
		u8 val = eeprom[MT_EE_WIFI_CONF];
		u8 band_sel = FIELD_GET(MT_EE_WIFI_CONF0_BAND_SEL, val);
		struct atenl_band *anb = &an->anb[0];

		/* MT7915A */
		if (band_sel == MT_EE_BAND_SEL_DEFAULT) {
			anb->valid = true;
			anb->cap = BAND_TYPE_2G_5G;
			return;
		}

		/* MT7915D */
		if (band_sel == MT_EE_BAND_SEL_2GHZ) {
			anb->valid = true;
			anb->cap = BAND_TYPE_2G;
		}

		val = eeprom[MT_EE_WIFI_CONF + 1];
		band_sel = FIELD_GET(MT_EE_WIFI_CONF0_BAND_SEL, val);
		anb++;

		if (band_sel == MT_EE_BAND_SEL_5GHZ) {
			anb->valid = true;
			anb->cap = BAND_TYPE_5G;
		}
	} else if (is_mt7916(an) || is_mt7981(an) || is_mt7986(an)) {
		struct atenl_band *anb;
		u8 val, band_sel;
		int i;

		for (i = 0; i < 2; i++) {
			val = eeprom[MT_EE_WIFI_CONF + i];
			band_sel = FIELD_GET(MT_EE_WIFI_CONF0_BAND_SEL, val);
			anb = &an->anb[i];

			anb->valid = true;
			switch (band_sel) {
			case MT_EE_BAND_SEL_2G:
				anb->cap = BAND_TYPE_2G;
				break;
			case MT_EE_BAND_SEL_5G:
				anb->cap = BAND_TYPE_5G;
				break;
			case MT_EE_BAND_SEL_6G:
				anb->cap = BAND_TYPE_6G;
				break;
			case MT_EE_BAND_SEL_5G_6G:
				anb->cap = BAND_TYPE_5G_6G;
				break;
			default:
				break;
			}
		}
	} else if (is_mt7996(an)) {
		struct atenl_band *anb;
		u8 val, band_sel;
		u8 band_sel_mask[3] = {EAGLE_BAND_SEL(0), EAGLE_BAND_SEL(1),
				       EAGLE_BAND_SEL(2)};
		int i;

		for (i = 0; i < 3; i++) {
			val = eeprom[MT_EE_WIFI_CONF + i];
			band_sel = FIELD_GET(band_sel_mask[i], val);
			anb = &an->anb[i];

			anb->valid = true;
			switch (band_sel) {
			case MT_EE_EAGLE_BAND_SEL_2GHZ:
				anb->cap = BAND_TYPE_2G;
				break;
			case MT_EE_EAGLE_BAND_SEL_5GHZ:
				anb->cap = BAND_TYPE_5G;
				break;
			case MT_EE_EAGLE_BAND_SEL_6GHZ:
				anb->cap = BAND_TYPE_6G;
				break;
			case MT_EE_EAGLE_BAND_SEL_5GHZ_6GHZ:
				anb->cap = BAND_TYPE_5G_6G;
				break;
			default:
				break;
			}
		}
	} else if (is_mt7992(an)) {
		struct atenl_band *anb;
		u8 val, band_sel;
		u8 band_sel_mask[2] = {EAGLE_BAND_SEL(0), EAGLE_BAND_SEL(1)};
		int i;

		for (i = 0; i < 2; i++) {
			val = eeprom[MT_EE_WIFI_CONF + i];
			band_sel = FIELD_GET(band_sel_mask[i], val);
			anb = &an->anb[i];

			anb->valid = true;
			switch (band_sel) {
			case MT_EE_EAGLE_BAND_SEL_2GHZ:
				anb->cap = BAND_TYPE_2G;
				break;
			case MT_EE_EAGLE_BAND_SEL_5GHZ:
				anb->cap = BAND_TYPE_5G;
				break;
			case MT_EE_EAGLE_BAND_SEL_6GHZ:
				anb->cap = BAND_TYPE_6G;
				break;
			case MT_EE_EAGLE_BAND_SEL_5GHZ_6GHZ:
				anb->cap = BAND_TYPE_5G_6G;
				break;
			default:
				break;
			}
		}
	}
}

static void
atenl_eeprom_init_antenna_cap(struct atenl *an)
{
	switch (an->chip_id) {
	case 0x7915:
		if (an->anb[0].cap == BAND_TYPE_2G_5G)
			an->anb[0].chainmask = 0xf;
		else {
			an->anb[0].chainmask = 0x3;
			an->anb[1].chainmask = 0xc;
		}
		break;
	case 0x7906:
	case 0x7916:
		an->anb[0].chainmask = 0x3;
		an->anb[1].chainmask = 0x3;
		break;
	case 0x7981:
		an->anb[0].chainmask = 0x3;
		an->anb[1].chainmask = 0x7;
		break;
	case 0x7986:
		an->anb[0].chainmask = 0xf;
		an->anb[1].chainmask = 0xf;
		break;
	case 0x7990:
		an->anb[0].chainmask = 0xf;
		an->anb[1].chainmask = 0xf;
		an->anb[2].chainmask = 0xf;
		break;
	case 0x7992:
		an->anb[0].chainmask = 0xf;
		an->anb[1].chainmask = 0xf;
		break;
	default:
		break;
	}
}

int atenl_eeprom_init(struct atenl *an, u8 phy_idx)
{
	bool flash_mode;
	int eeprom_fd;
	char buf[30];
	u8 main_phy_idx = phy_idx;

	set_band_val(an, 0, phy_idx, phy_idx);
	atenl_nl_check_mtd(an);
	flash_mode = an->mtd_part != NULL;

	// Get the first main phy index for this chip
	if (flash_mode)
		main_phy_idx -= an->band_idx;

	snprintf(buf, sizeof(buf), "/tmp/atenl-eeprom-phy%u", main_phy_idx);
	eeprom_file = strdup(buf);

	eeprom_fd = atenl_eeprom_init_file(an, flash_mode);
	if (eeprom_fd < 0)
		return -1;

	an->eeprom_data = mmap(NULL, EEPROM_PART_SIZE, PROT_READ | PROT_WRITE,
			       MAP_SHARED, eeprom_fd, 0);
	if (!an->eeprom_data) {
		perror("mmap");
		close(eeprom_fd);
		return -1;
	}

	an->eeprom_fd = eeprom_fd;
	atenl_eeprom_init_chip_id(an);
	atenl_eeprom_init_max_size(an);
	atenl_eeprom_init_band_cap(an);
	atenl_eeprom_init_antenna_cap(an);

	if (get_band_val(an, 1, valid))
		set_band_val(an, 1, phy_idx, phy_idx + 1);

	if (get_band_val(an, 2, valid))
		set_band_val(an, 2, phy_idx, phy_idx + 2);

	return 0;
}

void atenl_eeprom_close(struct atenl *an)
{
	msync(an->eeprom_data, EEPROM_PART_SIZE, MS_SYNC);
	munmap(an->eeprom_data, EEPROM_PART_SIZE);
	close(an->eeprom_fd);

	if (!an->cmd_mode) {
		if (remove(eeprom_file))
			perror("remove");
	}

	free(eeprom_file);
}

int atenl_eeprom_update_precal(struct atenl *an, int write_offs, int size)
{
	u32 offs = an->eeprom_prek_offs;
	u8 cal_indicator, *eeprom, *pre_cal;

	if (!an->cal && !an->cal_info)
		return 0;

	eeprom = an->eeprom_data;
	pre_cal = eeprom + an->eeprom_size;
	cal_indicator = an->cal_info[4];

	memcpy(eeprom + offs, &cal_indicator, sizeof(u8));
	memcpy(pre_cal, an->cal_info, PRE_CAL_INFO);
	pre_cal += (PRE_CAL_INFO + write_offs);

	if (an->cal)
		memcpy(pre_cal, an->cal, size);
	else
		memset(pre_cal, 0, size);

	return 0;
}

int atenl_eeprom_write_mtd(struct atenl *an)
{
#define TMP_FILE	"/tmp/tmp_eeprom.bin"
	pid_t pid;
	u32 size = an->eeprom_size;
	u32 *precal_info = an->eeprom_data + an->eeprom_size;
	u32 precal_size = precal_info[0] + precal_info[1];
	char cmd[100];

	if (an->mtd_part == NULL || !(~an->mtd_offset))
		return 0;

	if (precal_size)
		size += PRE_CAL_INFO + precal_size;

	sprintf(cmd, "dd if=%s of=%s bs=1 count=%d", eeprom_file, TMP_FILE, size);
	system(cmd);

	sprintf(cmd, "mtd -p %d write %s %s", an->mtd_offset, TMP_FILE, an->mtd_part);
	system(cmd);

	sprintf(cmd, "rm %s", TMP_FILE);
	system(cmd);

	return 0;
}

/* Directly read values from driver's eeprom.
 * It's usally used to get calibrated data from driver.
 */
int atenl_eeprom_read_from_driver(struct atenl *an, u32 offset, int len)
{
	u8 *eeprom_data = an->eeprom_data + offset;
	char fname[64], buf[1024];
	int fd_ori, ret;
	ssize_t rd;

	snprintf(fname, sizeof(fname),
		"/sys/kernel/debug/ieee80211/phy%d/mt76/eeprom",
		get_band_val(an, 0, phy_idx));
	fd_ori = open(fname, O_RDONLY);
	if (fd_ori < 0)
		return -1;

	ret = lseek(fd_ori, offset, SEEK_SET);
	if (ret < 0)
		goto out;

	while ((rd = read(fd_ori, buf, sizeof(buf))) > 0 && len) {
		if (len < rd) {
			memcpy(eeprom_data, buf, len);
			break;
		} else {
			memcpy(eeprom_data, buf, rd);
			eeprom_data += rd;
			len -= rd;
		}
	}

	ret = 0;
out:
	close(fd_ori);
	return ret;
}

/* Update all eeprom values to driver before writing efuse */
static void
atenl_eeprom_sync_to_driver(struct atenl *an)
{
	int i;

	for (i = 0; i < an->eeprom_size; i += 16)
		atenl_nl_write_eeprom(an, i, &an->eeprom_data[i], 16);
}

void atenl_eeprom_cmd_handler(struct atenl *an, u8 phy_idx, char *cmd)
{
	an->cmd_mode = true;

	atenl_eeprom_init(an, phy_idx);

	if (!strncmp(cmd, "sync eeprom all", 15)) {
		atenl_eeprom_write_mtd(an);
	} else if (!strncmp(cmd, "eeprom", 6)) {
		char *s = strchr(cmd, ' ');

		if (!s) {
			atenl_err("eeprom: please type a correct command\n");
			return;
		}

		s++;
		if (!strncmp(s, "reset", 5)) {
			unlink(eeprom_file);
		} else if (!strncmp(s, "file", 4)) {
			atenl_info("%s\n", eeprom_file);
			if (an->mtd_part != NULL)
				atenl_info("%s mode\n",
					   ~an->mtd_offset == 0 ? "Binfile" : "Flash");
			else
				atenl_info("Efuse / Default bin mode\n");
		} else if (!strncmp(s, "set", 3)) {
			u32 offset, val;

			s = strchr(s, ' ');
			if (!s)
				return;
			s++;

			if (!sscanf(s, "%x=%x", &offset, &val) ||
			    offset > EEPROM_PART_SIZE)
				return;

			an->eeprom_data[offset] = val;
			atenl_info("set offset 0x%x to 0x%x\n", offset, val);
		} else if (!strncmp(s, "update buffermode", 17)) {
			atenl_eeprom_sync_to_driver(an);
			atenl_nl_update_buffer_mode(an);
		} else if (!strncmp(s, "write", 5)) {
			s = strchr(s, ' ');
			if (!s)
				return;
			s++;

			if (!strncmp(s, "flash", 5)) {
				atenl_eeprom_write_mtd(an);
			} else if (!strncmp(s, "to efuse", 8)) {
				atenl_eeprom_sync_to_driver(an);
				atenl_nl_write_efuse_all(an);
			}
		} else if (!strncmp(s, "read", 4)) {
			u32 offset;

			s = strchr(s, ' ');
			if (!s)
				return;
			s++;

			if (!sscanf(s, "%x", &offset) ||
			    offset > EEPROM_PART_SIZE)
				return;

			atenl_info("val = 0x%x (%u)\n", an->eeprom_data[offset],
							an->eeprom_data[offset]);
		} else if (!strncmp(s, "precal", 6)) {
			s = strchr(s, ' ');
			if (!s)
				return;
			s++;

			if (!strncmp(s, "sync group", 10)) {
				atenl_nl_precal_sync_from_driver(an, PREK_SYNC_GROUP);
			} else if (!strncmp(s, "sync dpd 2g", 11)) {
				atenl_nl_precal_sync_from_driver(an, PREK_SYNC_DPD_2G);
			} else if (!strncmp(s, "sync dpd 5g", 11)) {
				atenl_nl_precal_sync_from_driver(an, PREK_SYNC_DPD_5G);
			} else if (!strncmp(s, "sync dpd 6g", 11)) {
				atenl_nl_precal_sync_from_driver(an, PREK_SYNC_DPD_6G);
			} else if (!strncmp(s, "group clean", 11)) {
				atenl_nl_precal_sync_from_driver(an, PREK_CLEAN_GROUP);
			} else if (!strncmp(s, "dpd clean", 9)) {
				atenl_nl_precal_sync_from_driver(an, PREK_CLEAN_DPD);
			} else if (!strncmp(s, "sync", 4)) {
				atenl_nl_precal_sync_from_driver(an, PREK_SYNC_ALL);
			}
		} else if (!strncmp(s, "ibf sync", 8)) {
			atenl_get_ibf_cal_result(an);
		} else {
			atenl_err("Unknown eeprom command: %s\n", cmd);
		}
	} else {
		atenl_err("Unknown command: %s\n", cmd);
	}
}
