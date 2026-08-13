// SPDX-License-Identifier: GPL-2.0-only
/*
 * MSI Claw 8 EX AI+ RT721 audio power workaround.
 *
 * The power sequences were recovered from the exact MSI 1462:1510 Windows
 * audio package. No vendor code or data is included in this module.
 */

#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/soundwire/sdw.h>
#include <linux/soundwire/sdw_type.h>

#define CLAW_SDW_NAME "sdw:0:3:025d:0721:01"
#define RT721_CODEC_POWER_REG 0x05f00000
#define RT721_LDO1_POWER_REG 0x00100011
#define RT721_BG1_POWER_REG 0x0010000c
#define RT721_LDO2_POWER_REG 0x00100013
#define RT721_BG2_POWER_REG 0x0010000e
#define RT721_AMP_POWER_REG 0x05f00020
#define RT721_MIC_POWER_REG 0x05f00030
#define RT721_HID_REG 0x0610001a
#define RT721_POWER_MASK 0xfe35

/* Exact order from the model-specific amplifier and microphone D0 data. */
static const unsigned int rt721_d0_bits[] = {
	BIT(11),
	BIT(10),
	BIT(5),
	BIT(0),
	BIT(9),
	BIT(4),
	BIT(2),
	BIT(12),
	BIT(13),
	BIT(14),
	BIT(15),
};

static bool apply;
module_param(apply, bool, 0400);
MODULE_PARM_DESC(apply, "Set the MSI RT721 amplifier D0 power bits");

static bool apply_mic;
module_param(apply_mic, bool, 0400);
MODULE_PARM_DESC(apply_mic, "Set the MSI RT721 microphone D0 power bits");

static bool apply_codec;
module_param(apply_codec, bool, 0400);
MODULE_PARM_DESC(apply_codec, "Set the MSI RT721 codec-wide D0 power bits");

static struct regmap *claw_regmap;
static struct device *rt721_dev;
static bool pm_acquired;

static const struct regmap_config claw_regmap_config = {
	.name = "claw-rt721-fix",
	.reg_bits = 32,
	.val_bits = 16,
	.max_register = RT721_HID_REG,
	.cache_type = REGCACHE_NONE,
	.use_single_read = true,
	.use_single_write = true,
};

static bool claw_dmi_matches(void)
{
	const char *product = dmi_get_system_info(DMI_PRODUCT_NAME);
	const char *board = dmi_get_system_info(DMI_BOARD_NAME);

	return product && board &&
		!strcmp(product, "Claw 8 EX AI+ CG3EM") &&
		!strcmp(board, "MS-1T91");
}

struct claw_rt721_reg_update {
	unsigned int reg;
	unsigned int mask;
	unsigned int value;
};

struct claw_rt721_reg_target {
	unsigned int reg;
	unsigned int mask;
	unsigned int value;
	const char *name;
	bool verify;
};

static const struct claw_rt721_reg_update rt721_codec_d0_steps[] = {
	{ RT721_CODEC_POWER_REG, BIT(15), BIT(15) },
	{ RT721_CODEC_POWER_REG, BIT(14), BIT(14) },
	{ RT721_CODEC_POWER_REG, BIT(13), BIT(13) },
	{ RT721_CODEC_POWER_REG, BIT(12), BIT(12) },
	{ RT721_LDO1_POWER_REG, BIT(15), BIT(15) },
	{ RT721_BG1_POWER_REG, BIT(15), BIT(15) },
	{ RT721_CODEC_POWER_REG, BIT(3), BIT(3) },
	{ RT721_CODEC_POWER_REG, BIT(10), BIT(10) },
	{ RT721_LDO2_POWER_REG, BIT(6), BIT(6) },
	{ RT721_BG2_POWER_REG, BIT(3), BIT(3) },
	{ RT721_CODEC_POWER_REG, BIT(2), BIT(2) },
	{ RT721_CODEC_POWER_REG, BIT(9), BIT(9) },
	{ RT721_HID_REG, BIT(15), BIT(15) },
	{ RT721_HID_REG, BIT(15), 0 },
};

static const struct claw_rt721_reg_target rt721_codec_d0_targets[] = {
	{ RT721_CODEC_POWER_REG, 0xf60c, 0xf60c, "codec", true },
	{ RT721_LDO1_POWER_REG, BIT(15), BIT(15), "ldo1", true },
	{ RT721_BG1_POWER_REG, BIT(15), BIT(15), "bg1", true },
	{ RT721_LDO2_POWER_REG, BIT(6), BIT(6), "ldo2", true },
	{ RT721_BG2_POWER_REG, BIT(3), BIT(3), "bg2", true },
	/* This trigger bit is hardware-owned and may reassert immediately. */
	{ RT721_HID_REG, BIT(15), 0, "hid", false },
};

static int claw_rt721_codec_power(bool do_apply)
{
	unsigned int before[ARRAY_SIZE(rt721_codec_d0_targets)];
	unsigned int after, expected;
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(rt721_codec_d0_targets); i++) {
		ret = regmap_read(claw_regmap, rt721_codec_d0_targets[i].reg,
				  &before[i]);
		if (ret) {
			pr_err("claw_rt721_amp: %s read 0x%08x failed: %d\n",
			       rt721_codec_d0_targets[i].name,
			       rt721_codec_d0_targets[i].reg, ret);
			return ret;
		}
		pr_info("claw_rt721_amp: %s 0x%08x before=0x%04x apply=%d\n",
			rt721_codec_d0_targets[i].name,
			rt721_codec_d0_targets[i].reg, before[i], do_apply);
	}

	if (!do_apply)
		return 0;

	for (i = 0; i < ARRAY_SIZE(rt721_codec_d0_steps); i++) {
		ret = regmap_update_bits(claw_regmap,
					 rt721_codec_d0_steps[i].reg,
					 rt721_codec_d0_steps[i].mask,
					 rt721_codec_d0_steps[i].value);
		if (ret) {
			pr_err("claw_rt721_amp: codec D0 step %u failed: %d\n",
			       i, ret);
			return ret;
		}
	}

	for (i = 0; i < ARRAY_SIZE(rt721_codec_d0_targets); i++) {
		ret = regmap_read(claw_regmap, rt721_codec_d0_targets[i].reg,
				  &after);
		if (ret) {
			pr_err("claw_rt721_amp: %s verification read failed: %d\n",
			       rt721_codec_d0_targets[i].name, ret);
			return ret;
		}
		expected = rt721_codec_d0_targets[i].value;
		pr_info("claw_rt721_amp: %s 0x%08x after=0x%04x expected-mask=0x%04x\n",
			rt721_codec_d0_targets[i].name,
			rt721_codec_d0_targets[i].reg, after, expected);
		if (rt721_codec_d0_targets[i].verify &&
		    (after & rt721_codec_d0_targets[i].mask) != expected) {
			pr_err("claw_rt721_amp: %s verification mismatch\n",
			       rt721_codec_d0_targets[i].name);
			return -EIO;
		}
	}

	return 0;
}

static int claw_rt721_path_power(unsigned int reg, const char *path,
				bool do_apply)
{
	unsigned int before, after;
	unsigned int i;
	int ret;

	ret = regmap_read(claw_regmap, reg, &before);
	if (ret) {
		pr_err("claw_rt721_amp: %s read 0x%08x failed: %d\n",
		       path, reg, ret);
		return ret;
	}

	pr_info("claw_rt721_amp: %s 0x%08x before=0x%04x apply=%d\n",
		path, reg, before, do_apply);

	if (!do_apply)
		return 0;

	for (i = 0; i < ARRAY_SIZE(rt721_d0_bits); i++) {
		ret = regmap_update_bits(claw_regmap, reg,
					 rt721_d0_bits[i], rt721_d0_bits[i]);
		if (ret) {
			pr_err("claw_rt721_amp: %s D0 bit %lu update failed: %d\n",
			       path, __ffs(rt721_d0_bits[i]), ret);
			return ret;
		}
	}

	ret = regmap_read(claw_regmap, reg, &after);
	if (ret) {
		pr_err("claw_rt721_amp: %s verification read failed: %d\n",
		       path, ret);
		return ret;
	}

	pr_info("claw_rt721_amp: %s 0x%08x after=0x%04x (mask=0x%04x)\n",
		path, reg, after, RT721_POWER_MASK);
	if ((after & RT721_POWER_MASK) != RT721_POWER_MASK) {
		pr_err("claw_rt721_amp: %s verification mismatch\n", path);
		return -EIO;
	}

	return 0;
}

static int __init claw_rt721_amp_init(void)
{
	struct sdw_slave *slave;
	int ret;

	if (!claw_dmi_matches()) {
		pr_err("claw_rt721_amp: refusing non-Claw MS-1T91 system\n");
		return -ENODEV;
	}

	rt721_dev = bus_find_device_by_name(&sdw_bus_type, NULL, CLAW_SDW_NAME);
	if (!rt721_dev) {
		pr_err("claw_rt721_amp: RT721 SoundWire device not found\n");
		return -ENODEV;
	}

	ret = pm_runtime_resume_and_get(rt721_dev);
	if (ret < 0) {
		pr_err("claw_rt721_amp: failed to runtime-resume RT721: %d\n",
		       ret);
		goto err_put_device;
	}
	pm_acquired = true;

	slave = dev_to_sdw_dev(rt721_dev);
	claw_regmap = regmap_init_sdw_mbq(slave, &claw_regmap_config);
	if (IS_ERR(claw_regmap)) {
		ret = PTR_ERR(claw_regmap);
		claw_regmap = NULL;
		pr_err("claw_rt721_amp: temporary MBQ regmap failed: %d\n", ret);
		goto err_put_device;
	}

	ret = claw_rt721_codec_power(apply_codec);
	if (ret)
		goto err_exit_regmap;

	ret = claw_rt721_path_power(RT721_AMP_POWER_REG, "amp", apply);
	if (ret)
		goto err_exit_regmap;

	ret = claw_rt721_path_power(RT721_MIC_POWER_REG, "mic", apply_mic);
	if (ret)
		goto err_exit_regmap;

	return 0;

err_exit_regmap:
	regmap_exit(claw_regmap);
	claw_regmap = NULL;
err_put_device:
	if (pm_acquired) {
		pm_runtime_mark_last_busy(rt721_dev);
		pm_runtime_put_autosuspend(rt721_dev);
		pm_acquired = false;
	}
	put_device(rt721_dev);
	rt721_dev = NULL;
	return ret;
}

static void __exit claw_rt721_amp_exit(void)
{
	if (claw_regmap)
		regmap_exit(claw_regmap);
	if (pm_acquired) {
		pm_runtime_mark_last_busy(rt721_dev);
		pm_runtime_put_autosuspend(rt721_dev);
		pm_acquired = false;
	}
	if (rt721_dev)
		put_device(rt721_dev);
	pr_info("claw_rt721_amp: unloaded; hardware value left unchanged\n");
}

module_init(claw_rt721_amp_init);
module_exit(claw_rt721_amp_exit);

MODULE_DESCRIPTION("MSI Claw 8 EX AI+ RT721 audio power workaround");
MODULE_AUTHOR("stevedamnvan and contributors");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");
