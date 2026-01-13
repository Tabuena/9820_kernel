/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Central definition of CPUCL2 DVFS overrides for Exynos9820 based platforms.
 *
 * This keeps CPUCL2 override data in one place, similar to the GPU overrides,
 * so ECT/FVMap and vclk consumers can stay in sync.
 */
#ifndef __CPUCL2_DVFS_OVERRIDES_H__
#define __CPUCL2_DVFS_OVERRIDES_H__

#include <linux/kernel.h>
#include <soc/samsung/ect_parser.h>

#include "cpucl2_dvfs_table.h"

#ifndef CPUCL2_PLL_RATE_TABLE_ONLY
struct cpucl2_dvfs_override_entry {
	unsigned long rate_khz;
	unsigned int volt_uv;
};

#define CPUCL2_OVERRIDE_ENTRY(rate_khz, volt_uv, pll_freq_hz, p, m, s, k) \
	{ rate_khz, volt_uv },
static const struct cpucl2_dvfs_override_entry cpucl2_dvfs_overrides[]
	__maybe_unused = {
	CPUCL2_DVFS_TABLE_ENTRY_LIST(CPUCL2_OVERRIDE_ENTRY)
};
#undef CPUCL2_OVERRIDE_ENTRY

static inline size_t cpucl2_dvfs_override_count(void)
{
	return ARRAY_SIZE(cpucl2_dvfs_overrides);
}

static inline bool cpucl2_dvfs_has_overrides(void)
{
	return cpucl2_dvfs_override_count() > 0;
}

static inline unsigned long cpucl2_dvfs_override_highest_rate(void)
{
	unsigned long rate = 0;
	size_t i;

	for (i = 0; i < cpucl2_dvfs_override_count(); i++)
		rate = max(rate, cpucl2_dvfs_overrides[i].rate_khz);

	return rate;
}

static inline const struct cpucl2_dvfs_override_entry *
cpucl2_dvfs_override_lookup(unsigned long rate_khz)
{
	size_t i;

	for (i = 0; i < cpucl2_dvfs_override_count(); i++) {
		if (cpucl2_dvfs_overrides[i].rate_khz == rate_khz)
			return &cpucl2_dvfs_overrides[i];
	}

	return NULL;
}

static inline const struct cpucl2_dvfs_override_entry *
cpucl2_dvfs_override_get(size_t index)
{
	if (index >= cpucl2_dvfs_override_count())
		return NULL;

	return &cpucl2_dvfs_overrides[index];
}

static const struct ect_minlock_frequency cpucl2_minlock_levels[]
	__maybe_unused = {
	{ 4000000, 1352000 },
	{ 3900000, 1352000 },
	{ 3800000, 1352000 },
	{ 3700000, 1352000 },
	{ 3600000, 1352000 },
	{ 3500000, 1352000 },
	{ 3400000, 1352000 },
	{ 3300000, 1352000 },
	{ 3200000, 1352000 },
	{ 3102000, 1352000 },
	{ 3016000, 1352000 },
	{ 2912000, 1352000 },
	{ 2808000, 1352000 },
	{ 2730000, 1352000 },
	{ 2600000, 1352000 },
	{ 2530000, 1352000 },
	{ 2470000, 1352000 },
	{ 2340000, 1352000 },
	{ 2236000, 1352000 },
	{ 2080000, 1014000 },
	{ 1976000, 1014000 },
	{ 1820000, 1014000 },
	{ 1664000, 845000 },
	{ 1560000, 845000 },
	{ 1456000, 845000 },
	{ 1378000, 845000 },
	{ 1248000, 676000 },
	{ 1144000, 676000 },
	{ 1040000, 421000 },
	{ 936000, 421000 },
	{ 819000, 421000 },
	{ 728000, 421000 },
	{ 624000, 421000 },
	{ 520000, 421000 },
};

static inline size_t cpucl2_minlock_level_count(void)
{
	return ARRAY_SIZE(cpucl2_minlock_levels);
}

static inline const struct ect_minlock_frequency *
cpucl2_minlock_level_list(void)
{
	return cpucl2_minlock_levels;
}
#endif /* CPUCL2_PLL_RATE_TABLE_ONLY */

#ifdef CPUCL2_PLL_RATE_TABLE_DEFINE
#include "cmucal.h"

#define CPUCL2_PLL_RATE_ENTRY(rate_khz, volt_uv, pll_freq_hz, p, m, s, k) \
	PLL_RATE_MPS(pll_freq_hz, m, p, s),
static struct cmucal_pll_table pll_cpucl2_rate_table[] = {
	CPUCL2_DVFS_TABLE_ENTRY_LIST(CPUCL2_PLL_RATE_ENTRY)
};
#undef CPUCL2_PLL_RATE_ENTRY
#endif /* CPUCL2_PLL_RATE_TABLE_DEFINE */

#endif /* __CPUCL2_DVFS_OVERRIDES_H__ */
