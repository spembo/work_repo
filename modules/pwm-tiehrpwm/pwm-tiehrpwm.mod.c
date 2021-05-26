#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xbc2b92d7, "module_layout" },
	{ 0xf4b6b043, "platform_driver_unregister" },
	{ 0xe78e4470, "__platform_driver_register" },
	{ 0xe707d823, "__aeabi_uidiv" },
	{ 0xd8917bdf, "of_device_is_compatible" },
	{ 0x4ebd1401, "pm_runtime_enable" },
	{ 0x9bcf5856, "pwmchip_add" },
	{ 0x7c9a7371, "clk_prepare" },
	{ 0x6b4ca18c, "devm_ioremap_resource" },
	{ 0x71e3d694, "platform_get_resource" },
	{ 0xc5def423, "of_pwm_xlate_with_flags" },
	{ 0x556e4390, "clk_get_rate" },
	{ 0xebfbe85b, "devm_clk_get" },
	{ 0xb3a78a67, "devm_kmalloc" },
	{ 0x247e59a6, "_dev_err" },
	{ 0x815588a6, "clk_enable" },
	{ 0xb6e6d99d, "clk_disable" },
	{ 0xa5dab080, "_dev_warn" },
	{ 0xe44959e, "pwmchip_remove" },
	{ 0x95a9bfcb, "__pm_runtime_disable" },
	{ 0xb077e70a, "clk_unprepare" },
	{ 0xab936045, "__pm_runtime_idle" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xe6ed9373, "__pm_runtime_resume" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cti,am3352-ehrpwm");
MODULE_ALIAS("of:N*T*Cti,am3352-ehrpwmC*");
MODULE_ALIAS("of:N*T*Cti,am33xx-ehrpwm");
MODULE_ALIAS("of:N*T*Cti,am33xx-ehrpwmC*");
