/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Central definition of CPUCL1 DVFS overrides for Exynos9820 based platforms.
 *
 * This keeps CPUCL1 override data in one place, similar to the GPU overrides,
 * so ECT/FVMap and vclk consumers can stay in sync.
 */
#ifndef __CPUCL1_DVFS_OVERRIDES_H__
#define __CPUCL1_DVFS_OVERRIDES_H__

#include <linux/kernel.h>

#include "cpucl1_dvfs_table.h"

#ifndef CPUCL1_PLL_RATE_TABLE_ONLY
struct cpucl1_dvfs_override_entry {
	unsigned long rate_khz;
	unsigned int volt_uv;
};

#define CPUCL1_OVERRIDE_ENTRY(rate_khz, volt_uv, pll_freq_hz, p, m, s, k) \
	{ rate_khz, volt_uv },
static const struct cpucl1_dvfs_override_entry cpucl1_dvfs_overrides[]
	__maybe_unused = {
	CPUCL1_DVFS_TABLE_ENTRY_LIST(CPUCL1_OVERRIDE_ENTRY)
};
#undef CPUCL1_OVERRIDE_ENTRY

static inline size_t cpucl1_dvfs_override_count(void)
{
	return ARRAY_SIZE(cpucl1_dvfs_overrides);
}

static inline bool cpucl1_dvfs_has_overrides(void)
{
	return cpucl1_dvfs_override_count() > 0;
}

static inline unsigned long cpucl1_dvfs_override_highest_rate(void)
{
	unsigned long rate = 0;
	size_t i;

	for (i = 0; i < cpucl1_dvfs_override_count(); i++)
		rate = max(rate, cpucl1_dvfs_overrides[i].rate_khz);

	return rate;
}

static inline const struct cpucl1_dvfs_override_entry *
cpucl1_dvfs_override_lookup(unsigned long rate_khz)
{
	size_t i;

	for (i = 0; i < cpucl1_dvfs_override_count(); i++) {
		if (cpucl1_dvfs_overrides[i].rate_khz == rate_khz)
			return &cpucl1_dvfs_overrides[i];
	}

	return NULL;
}

static inline const struct cpucl1_dvfs_override_entry *
cpucl1_dvfs_override_get(size_t index)
{
	if (index >= cpucl1_dvfs_override_count())
		return NULL;

	return &cpucl1_dvfs_overrides[index];
}
#endif /* CPUCL1_PLL_RATE_TABLE_ONLY */

#ifdef CPUCL1_PLL_RATE_TABLE_DEFINE
#include "cmucal.h"

#define CPUCL1_PLL_RATE_ENTRY(rate_khz, volt_uv, pll_freq_hz, p, m, s, k) \
	PLL_RATE_MPS(pll_freq_hz, m, p, s),
static struct cmucal_pll_table pll_cpucl1_rate_table[] = {
	CPUCL1_DVFS_TABLE_ENTRY_LIST(CPUCL1_PLL_RATE_ENTRY)
};
#undef CPUCL1_PLL_RATE_ENTRY
#endif /* CPUCL1_PLL_RATE_TABLE_DEFINE */

#endif /* __CPUCL1_DVFS_OVERRIDES_H__ */
